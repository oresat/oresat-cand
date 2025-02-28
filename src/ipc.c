#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zmq.h>
#include "CANopen.h"
#include "CO_ODinterface.h"
#include "CO_SDOserver.h"
#include "sdo_client.h"
#include "logger.h"
#include "ipc.h"

#define ZMQ_HEADER_LEN 5
#define BUFFER_SIZE 1024

static void *context = NULL;
static void *responder = NULL;
static void *broadcaster = NULL;
static void *monitor = NULL;
static void *consumer = NULL;
static uint8_t clients = 0;

static ODR_t ipc_broadcast_data(OD_stream_t* stream, const void* buf, OD_size_t count, OD_size_t* countWritten);

static OD_extension_t ext = {
    .object = NULL,
    .read = NULL,
    .write = ipc_broadcast_data,
};

void ipc_init(OD_t *od) {
    context = zmq_ctx_new();

    responder = zmq_socket(context, ZMQ_ROUTER);
    zmq_bind(responder, "tcp://*:6000");

    broadcaster = zmq_socket(context, ZMQ_PUB);
    zmq_bind(broadcaster, "tcp://*:6001");

    zmq_socket_monitor(broadcaster, "inproc://monitor", ZMQ_EVENT_ACCEPTED | ZMQ_EVENT_DISCONNECTED);
    monitor = zmq_socket(context, ZMQ_PAIR);
    zmq_connect(monitor, "inproc://monitor");

    consumer = zmq_socket(context, ZMQ_SUB);
    zmq_setsockopt(consumer, ZMQ_SUBSCRIBE, NULL, 0);
    zmq_bind(consumer, "tcp://*:6002");

    for (int i=0; i<od->size; i++) {
        if (od->list[i].index >= 0x4000) {
            OD_extension_init(&od->list[i], &ext);
        }
    }
}

void ipc_responder_process(CO_t* co, OD_t* od, CO_config_t *config) {
    if (!co || !od || !config) {
        log_error("ipc process null arg");
        return;
    }

    static uint8_t buffer_in[BUFFER_SIZE];
    static uint8_t buffer_out[BUFFER_SIZE];

    uint8_t header[ZMQ_HEADER_LEN];

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
    } if (nbytes != ZMQ_HEADER_LEN) {
        log_error("zmq msg recv header error %d", errno);
        zmq_msg_close(&msg);
        return;
    } else if (nbytes != ZMQ_HEADER_LEN) {
        log_error("unexpected header len %d", nbytes);
        zmq_msg_close(&msg);
        return;
    }
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
    memcpy(&buffer_in, zmq_msg_data(&msg), nbytes);
    int buffer_in_recv = nbytes;

    zmq_msg_close(&msg);

    int buffer_out_send = 0;

    // default response
    buffer_out[0] = IPC_MSG_ID_ERROR;
    buffer_out_send = 1;

    switch (buffer_in[0]) {
        case IPC_MSG_ID_SDO_READ:
        {
            if (buffer_in_recv == sizeof(ipc_msg_sdo_t)) {
                ipc_msg_sdo_t msg_sdo;
                memcpy(&msg_sdo, buffer_in, sizeof(ipc_msg_sdo_t));
                log_debug("sdo read node 0x%X index 0x%X subindex 0x%X", msg_sdo.node_id, msg_sdo.index, msg_sdo.subindex);
                void *data = NULL;
                size_t data_len = 0;
                CO_SDO_abortCode_t ac = -1;
                if (co->SDOclient) {
                    ac = sdo_read_dynamic(co->SDOclient, msg_sdo.node_id, msg_sdo.index, msg_sdo.subindex, &data, &data_len);
                }
                if (ac == 0) {
                    if (data == NULL) {
                        memcpy(buffer_out, buffer_in, buffer_in_recv);
                        buffer_out_send = buffer_in_recv;
                    } else if ((buffer_in_recv + data_len) > BUFFER_SIZE) {
                        free(data);
                        ipc_msg_error_abort_t msg_error_abort = {
                            .id = IPC_MSG_ID_ERROR_ABORT,
                            .abort_code = CO_SDO_AB_DATA_LONG,
                        };
                        buffer_out_send = sizeof(ipc_msg_error_abort_t);
                        memcpy(buffer_out, &msg_error_abort, buffer_out_send);
                    } else {
                        memcpy(buffer_out, buffer_in, buffer_in_recv);
                        memcpy(&buffer_out[buffer_in_recv], data, data_len);
                        buffer_out_send = buffer_in_recv + data_len;
                        free(data);
                    }
                } else {
                    ipc_msg_error_abort_t msg_error_abort = {
                        .id = IPC_MSG_ID_ERROR_ABORT,
                        .abort_code = ac,
                    };
                    buffer_out_send = sizeof(ipc_msg_error_abort_t);
                    memcpy(buffer_out, &msg_error_abort, buffer_out_send);
                }
            } else {
                log_error("unexpected length for sdo read message: %d", buffer_in_recv);
            }
            break;
        }
        case IPC_MSG_ID_SDO_WRITE:
        {
            if (buffer_in_recv > (int)sizeof(ipc_msg_sdo_t)) {
                ipc_msg_sdo_t msg_sdo;
                memcpy(&msg_sdo, buffer_in, sizeof(ipc_msg_sdo_t));
                log_debug("sdo write node 0x%X index 0x%X subindex 0x%X", msg_sdo.node_id, msg_sdo.index, msg_sdo.subindex);
                CO_SDO_abortCode_t ac = -1;
                if (co->SDOclient) {
                    uint8_t *data = &buffer_in[sizeof(ipc_msg_sdo_t)];
                    size_t data_len = buffer_in_recv - sizeof(ipc_msg_sdo_t);
                    ac = sdo_write(co->SDOclient, msg_sdo.node_id, msg_sdo.index, msg_sdo.subindex, data, data_len);
                }
                if (ac == 0) {
                    memcpy(buffer_out, buffer_in, buffer_in_recv);
                    buffer_out_send = buffer_in_recv;
                } else {
                    ipc_msg_error_abort_t msg_error_abort = {
                        .id = IPC_MSG_ID_ERROR_ABORT,
                        .abort_code = ac,
                    };
                    buffer_out_send = sizeof(ipc_msg_error_abort_t);
                    memcpy(buffer_out, &msg_error_abort, buffer_out_send);
                }
            } else {
                log_error("unexpected length for sdo write message: %d", buffer_in_recv);
            }
            break;
        }
        default:
        {
            log_debug("unknown msg id %d", buffer_in[0]);
            buffer_out[0] = IPC_MSG_ID_ERROR_UNKNOWN_ID;
            buffer_out_send = 1;
            break;
        }
    }

    zmq_send(responder, header, 5, ZMQ_SNDMORE);
    zmq_send(responder, header, 0, ZMQ_SNDMORE);
    zmq_send(responder, buffer_out, buffer_out_send, 0);
}

void ipc_consumer_process(CO_t* co, OD_t* od, CO_config_t *config) {
    if (!co || !od || !config) {
        log_error("ipc process null arg");
        return;
    }

    static uint8_t buffer_in[BUFFER_SIZE];
    int buffer_in_recv = 0;

    buffer_in_recv = zmq_recv(consumer, buffer_in, BUFFER_SIZE, 0);
    if (buffer_in_recv <= 0) {
        return;
    }

    switch (buffer_in[0]) {
        case IPC_MSG_ID_EMCY_SEND:
        {
            if (buffer_in_recv == sizeof(ipc_msg_emcy_t)) {
                ipc_msg_emcy_t msg_emcy;
                memcpy(&msg_emcy, buffer_in, sizeof(ipc_msg_emcy_t));
                log_info("emcy send with code 0x%X info 0x%X", msg_emcy.code, msg_emcy.info);
                CO_errorReport(co->em, CO_EM_GENERIC_ERROR, msg_emcy.code, msg_emcy.info);
            } else {
                log_error("consumer emcy send message size error");
            }
            break;
        }
        case IPC_MSG_ID_TPDO_SEND:
        {
            if (buffer_in_recv == sizeof(ipc_msg_tpdo_t)) {
                ipc_msg_tpdo_t msg_tpdo;
                memcpy(&msg_tpdo, buffer_in, sizeof(ipc_msg_tpdo_t));
                if (msg_tpdo.num < config->CNT_TPDO) {
                    log_debug("tpdo send %d", msg_tpdo.num);
                    co->TPDO[msg_tpdo.num].sendRequest = true;
                } else {
                    log_error("invalid tpdo number %d", msg_tpdo.num);
                }
            } else {
                log_error("consume tpdo send message size error");
            }
            break;
        }
        case IPC_MSG_ID_OD_WRITE:
        {
            if (buffer_in_recv > (int)sizeof(ipc_msg_od_t)) {
                ipc_msg_od_t msg_od;
                memcpy(&msg_od, buffer_in, sizeof(ipc_msg_od_t));
                log_debug("consume od write index 0x%X subindex 0x%X", msg_od.index, msg_od.subindex);
                OD_entry_t *entry = OD_find(od, msg_od.index);
                uint8_t *data = &buffer_in[sizeof(ipc_msg_od_t)];
                size_t data_len = buffer_in_recv - sizeof(ipc_msg_od_t);
                OD_set_value(entry, msg_od.subindex, data, data_len, false);
            } else {
                log_error("consume od write message size error");
            }
            break;
        }
        default:
        {
            log_debug("consume unknown msg id %d", buffer_in[0]);
            break;
        }
    }
}

void ipc_monitor_process(void) {
    zmq_msg_t msg;

    // event number and value
    zmq_msg_init(&msg);
    int r = zmq_msg_recv(&msg, monitor, 0);
    if (r == -1) {
        zmq_msg_close(&msg);
        return;
    }
    uint8_t *data = zmq_msg_data(&msg);
    uint16_t event = *(uint16_t *)data;
    if (event & ZMQ_EVENT_ACCEPTED) {
        clients++;
        log_info("new client connected (total %d)", clients);
    } else if (event & ZMQ_EVENT_DISCONNECTED) {
        clients--;
        log_info("client disconnected (total %d)", clients);
    }

    // event address
    zmq_msg_init(&msg);
    r = zmq_msg_recv(&msg, monitor, 0);
    if (r == -1) {
        zmq_msg_close(&msg);
        return;
    }

    zmq_msg_close(&msg);
}

void ipc_free(void) {
    zmq_close(responder);
    zmq_close(broadcaster);
    zmq_close(consumer);
    zmq_close(monitor);
    zmq_ctx_term(context);
}

static ODR_t ipc_broadcast_data(OD_stream_t* stream, const void* buf, OD_size_t count, OD_size_t* countWritten) {
    ODR_t r = OD_writeOriginal(stream, buf, count, countWritten);
    if ((r == ODR_OK) && (clients > 1) && (stream->dataOrig != NULL)) {
        ipc_msg_od_t msg_od = {
            .id = IPC_MSG_ID_OD_WRITE,
            .index = stream->index,
            .subindex = stream->subIndex,
        };
        int length = sizeof(ipc_msg_od_t) + sizeof(stream->dataLength);
        uint8_t *buf = malloc(length);
        if (buf == NULL) {
            log_error("malloc error");
        } else {
            memcpy(buf, &msg_od, sizeof(ipc_msg_od_t));
            uint8_t *data_orig = (uint8_t *)stream->dataOrig;
            memcpy(buf, &data_orig[sizeof(ipc_msg_od_t)], stream->dataLength);
            zmq_send(broadcaster, buf, length, 0);
            log_debug("broadcast od write index 0x%X subindex 0x%X", msg_od.index, msg_od.subindex);
            free(buf);
        }
    }
    return r;
}
