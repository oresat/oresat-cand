#include "ipc_respond.h"
#include "CANopen.h"
#include "ipc_msg.h"
#include "logger.h"
#include "sdo_client.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <zmq.h>

#define ZMQ_HEADER_LEN 5

static void *responder = NULL;

static uint32_t ipc_respond_sdo_read(uint8_t *buffer_in, uint32_t buffer_in_recv, uint8_t *buffer_out, CO_t *co);
static uint32_t ipc_respond_sdo_write(uint8_t *buffer_in, uint32_t buffer_in_recv, uint8_t *buffer_out, CO_t *co);
static uint32_t ipc_respond_add_file(uint8_t *buffer_in, uint32_t buffer_in_recv, uint8_t *buffer_out,
                                     fcache_t *fread_cache);
static uint32_t ipc_respond_sdo_read_to_file(uint8_t *buffer_in, uint32_t buffer_in_recv, uint8_t *buffer_out,
                                             CO_t *co);
static uint32_t ipc_respond_sdo_write_from_file(uint8_t *buffer_in, uint32_t buffer_in_recv, uint8_t *buffer_out,
                                                CO_t *co);

int ipc_respond_init(void *context) {
    if (!context) {
        return -EINVAL;
    }
    responder = zmq_socket(context, ZMQ_ROUTER);
    zmq_bind(responder, "tcp://*:6000");

    return 0;
}

void ipc_respond_process(CO_t *co, OD_t *od, CO_config_t *config, fcache_t *fread_cache) {
    if (!co || !od || !config) {
        log_error("null arg");
        return;
    }

    zmq_msg_t msg;
    int r = zmq_msg_init(&msg);
    if (r != 0) {
        return;
    }

    int nbytes = 0;
    nbytes = zmq_msg_recv(&msg, responder, 0);
    if (nbytes < 0) {
        zmq_msg_close(&msg);
        return;
    }
    if (nbytes != ZMQ_HEADER_LEN) {
        log_error("zmq msg recv header error %d", errno);
        zmq_msg_close(&msg);
        return;
    } else if (nbytes != ZMQ_HEADER_LEN) {
        log_error("unexpected header len %d", nbytes);
        zmq_msg_close(&msg);
        return;
    }
    uint8_t header[ZMQ_HEADER_LEN];
    memcpy(header, zmq_msg_data(&msg), nbytes);

    nbytes = zmq_msg_recv(&msg, responder, 0);
    if (nbytes != 0) {
        log_error("zmq msg recv header error");
        zmq_msg_close(&msg);
        return;
    }

    nbytes = zmq_msg_recv(&msg, responder, 0);
    if (nbytes == -1) {
        log_error("zmq msg recv error %d", errno);
        zmq_msg_close(&msg);
        return;
    }

    if (nbytes <= 2) {
        log_error("ipc msg is to small at %d bytes", nbytes);
        zmq_msg_close(&msg);
        return;
    }

    uint32_t buffer_in_recv = nbytes;
    static uint8_t buffer_in[IPC_MSG_MAX_LEN];
    memcpy(&buffer_in, zmq_msg_data(&msg), nbytes);

    zmq_msg_close(&msg);

    if (buffer_in[0] != IPC_MSG_VERSION) {
        log_error("expected ipc protocal version %d not %d", IPC_MSG_VERSION, buffer_in[0]);
        return;
    }

    uint32_t buffer_out_send = 0;
    static uint8_t buffer_out[IPC_MSG_MAX_LEN];

    switch (buffer_in[1]) {
    case IPC_MSG_ID_SDO_READ:
        buffer_out_send = ipc_respond_sdo_read(buffer_in, buffer_in_recv, buffer_out, co);
        break;
    case IPC_MSG_ID_SDO_WRITE:
        buffer_out_send = ipc_respond_sdo_write(buffer_in, buffer_in_recv, buffer_out, co);
        break;
    case IPC_MSG_ID_ADD_FILE:
        buffer_out_send = ipc_respond_add_file(buffer_in, buffer_in_recv, buffer_out, fread_cache);
        break;
    case IPC_MSG_ID_SDO_READ_TO_FILE:
        buffer_out_send = ipc_respond_sdo_read_to_file(buffer_in, buffer_in_recv, buffer_out, co);
        break;
    case IPC_MSG_ID_SDO_WRITE_FROM_FILE:
        buffer_out_send = ipc_respond_sdo_write_from_file(buffer_in, buffer_in_recv, buffer_out, co);
        break;
    default:
        log_debug("unknown msg id %d", buffer_in[0]);
        ipc_msg_error_id_t *msg_error_id = (ipc_msg_error_id_t *)buffer_out;
        msg_error_id->header.version = IPC_MSG_VERSION;
        msg_error_id->header.id = IPC_MSG_ID_ERROR_UNKNOWN_ID;
        msg_error_id->id = buffer_in[1];
        buffer_out_send = sizeof(ipc_msg_error_t);
        break;
    }

    // always send a response
    if (buffer_out_send == 0) {
        ipc_msg_error_t *msg_error = (ipc_msg_error_t *)buffer_out;
        msg_error->header.version = IPC_MSG_VERSION;
        msg_error->header.id = IPC_MSG_ID_ERROR;
        msg_error->error = EINVAL;
        buffer_out_send = sizeof(ipc_msg_error_t);
    }

    zmq_send(responder, header, ZMQ_HEADER_LEN, ZMQ_SNDMORE);
    zmq_send(responder, header, 0, ZMQ_SNDMORE);
    zmq_send(responder, buffer_out, buffer_out_send, 0);
}

void ipc_respond_free(void) {
    if (responder) {
        zmq_close(responder);
        responder = NULL;
    }
}

static uint32_t make_sdo_abort_msg(uint8_t *buffer_out, uint32_t abort_code) {
    ipc_msg_error_abort_t *msg_error_abort = (ipc_msg_error_abort_t *)buffer_out;
    msg_error_abort->header.version = IPC_MSG_VERSION;
    msg_error_abort->header.id = IPC_MSG_ID_ERROR_ABORT;
    msg_error_abort->code = abort_code;
    return sizeof(ipc_msg_error_abort_t);
}

static uint32_t ipc_respond_sdo_read(uint8_t *buffer_in, uint32_t buffer_in_recv, uint8_t *buffer_out, CO_t *co) {
    if (co->SDOclient) {
        log_error("node is not an sdo client");
        return 0;
    }
    if (buffer_in_recv == sizeof(ipc_msg_sdo_t)) {
        log_error("unexpected length for sdo read message: %d", buffer_in_recv);
        return 0;
    }

    uint32_t buffer_out_send = 0;
    ipc_msg_sdo_t *msg_sdo = (ipc_msg_sdo_t *)&buffer_in;
    log_debug("sdo read node 0x%X index 0x%X subindex 0x%X", msg_sdo->node_id, msg_sdo->index, msg_sdo->subindex);

    void *data = NULL;
    size_t data_len = 0;
    CO_SDO_abortCode_t ac =
        sdo_read_dynamic(co->SDOclient, msg_sdo->node_id, msg_sdo->index, msg_sdo->subindex, &data, &data_len, false);
    if (ac == CO_SDO_AB_NONE) {
        if (data == NULL) {
            memcpy(buffer_out, buffer_in, buffer_in_recv);
            buffer_out_send = buffer_in_recv;
        } else if ((buffer_in_recv + data_len) > IPC_MSG_MAX_LEN) {
            free(data);
            buffer_out_send = make_sdo_abort_msg(buffer_out, CO_SDO_AB_DATA_LONG);
        } else {
            memcpy(buffer_out, buffer_in, buffer_in_recv);
            memcpy(&buffer_out[buffer_in_recv], data, data_len);
            buffer_out_send = buffer_in_recv + data_len;
            free(data);
        }
    } else {
        buffer_out_send = make_sdo_abort_msg(buffer_out, ac);
    }
    return buffer_out_send;
}

static uint32_t ipc_respond_sdo_write(uint8_t *buffer_in, uint32_t buffer_in_recv, uint8_t *buffer_out, CO_t *co) {
    if (co->SDOclient) {
        log_error("node is not an sdo client");
        return 0;
    }
    if (buffer_in_recv <= (int)sizeof(ipc_msg_sdo_t)) {
        log_error("unexpected length for sdo write message: %d", buffer_in_recv);
        return 0;
    }

    uint32_t buffer_out_send = 0;
    ipc_msg_sdo_t *msg_sdo = (ipc_msg_sdo_t *)&buffer_in;
    log_debug("sdo write node 0x%X index 0x%X subindex 0x%X", msg_sdo->node_id, msg_sdo->index, msg_sdo->subindex);

    CO_SDO_abortCode_t ac = sdo_write(co->SDOclient, msg_sdo->node_id, msg_sdo->index, msg_sdo->subindex,
                                      msg_sdo->buffer.data, msg_sdo->buffer.len);
    if (ac == CO_SDO_AB_NONE) {
        memcpy(buffer_out, buffer_in, buffer_in_recv);
        buffer_out_send = buffer_in_recv;
    } else {
        buffer_out_send = make_sdo_abort_msg(buffer_out, ac);
    }
    return buffer_out_send;
}

static uint32_t ipc_respond_add_file(uint8_t *buffer_in, uint32_t buffer_in_recv, uint8_t *buffer_out,
                                     fcache_t *fread_cache) {
    if (!fread_cache) {
        log_error("node does not have a fread cache");
        return 0;
    }
    if (buffer_in_recv > sizeof(ipc_bytes_t)) {
        log_error("unexpected length for add file message: %d", buffer_in_recv);
        return 0;
    }

    uint32_t buffer_out_send = 0;
    ipc_msg_sdo_file_t *msg_sdo_file = (ipc_msg_sdo_file_t *)&buffer_in;
    msg_sdo_file->path.data[msg_sdo_file->path.len] = '\0';

    int error = fcache_add(fread_cache, msg_sdo_file->path.data, false);
    if (error) {
        buffer_out_send = buffer_in_recv;
        memcpy(buffer_out, buffer_in, buffer_out_send);
    } else {
        buffer_out[0] = IPC_MSG_ID_ERROR_ABORT;
        buffer_out[1] = (uint8_t)error;
        buffer_out_send = 2;
    }
    return buffer_out_send;
}

static uint32_t ipc_respond_sdo_read_to_file(uint8_t *buffer_in, uint32_t buffer_in_recv, uint8_t *buffer_out,
                                             CO_t *co) {
    if (co->SDOclient) {
        log_error("node is not an sdo client");
        return 0;
    }
    if ((buffer_in_recv < IPC_MSG_SDO_FILE_MIN_LEN) || (buffer_in_recv > sizeof(ipc_msg_sdo_file_t))) {
        log_error("msg len mismatch; got %d, expect between %d to %d", buffer_in_recv, IPC_MSG_SDO_FILE_MIN_LEN,
                  sizeof(ipc_msg_sdo_file_t));
        return 0;
    }

    ipc_msg_sdo_file_t *msg_sdo_file = (ipc_msg_sdo_file_t *)buffer_in;
    msg_sdo_file->path.data[msg_sdo_file->path.len] = '\0';
    log_debug("node 0x%X sdo read from index 0x%X subindex 0x%X to file %s", msg_sdo_file->node_id, msg_sdo_file->index,
              msg_sdo_file->subindex, msg_sdo_file->path.data);

    uint32_t buffer_out_send;
    CO_SDO_abortCode_t ac = sdo_read_to_file(co->SDOclient, msg_sdo_file->node_id, msg_sdo_file->index,
                                             msg_sdo_file->subindex, msg_sdo_file->path.data);
    if (ac == CO_SDO_AB_NONE) {
        memcpy(buffer_out, buffer_in, buffer_in_recv);
        buffer_out_send = buffer_in_recv;
    } else {
        buffer_out_send = make_sdo_abort_msg(buffer_out, ac);
    }
    return buffer_out_send;
}

static uint32_t ipc_respond_sdo_write_from_file(uint8_t *buffer_in, uint32_t buffer_in_recv, uint8_t *buffer_out,
                                                CO_t *co) {
    if (co->SDOclient) {
        log_error("node is not an sdo client");
        return 0;
    }
    if ((buffer_in_recv < IPC_MSG_SDO_FILE_MIN_LEN) || (buffer_in_recv > sizeof(ipc_msg_sdo_file_t))) {
        log_error("msg len mismatch; got %d, expect between %d to %d", buffer_in_recv, IPC_MSG_SDO_FILE_MIN_LEN,
                  sizeof(ipc_msg_sdo_file_t));
        return 0;
    }

    ipc_msg_sdo_file_t *msg_sdo_file = (ipc_msg_sdo_file_t *)buffer_in;
    msg_sdo_file->path.data[msg_sdo_file->path.len] = '\0';
    log_debug("node 0x%X sdo write to index 0x%X subindex 0x%X from file %s", msg_sdo_file->node_id,
              msg_sdo_file->index, msg_sdo_file->subindex, msg_sdo_file->path.data);

    uint32_t buffer_out_send;
    CO_SDO_abortCode_t ac = sdo_write_from_file(co->SDOclient, msg_sdo_file->node_id, msg_sdo_file->index,
                                                msg_sdo_file->subindex, msg_sdo_file->path.data);
    if (ac == CO_SDO_AB_NONE) {
        memcpy(buffer_out, buffer_in, buffer_in_recv);
        buffer_out_send = buffer_in_recv;
    } else {
        buffer_out_send = make_sdo_abort_msg(buffer_out, ac);
    }
    return buffer_out_send;
}
