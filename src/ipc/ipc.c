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
#include "od_ext.h"
#include "ipc.h"

#define ZMQ_HEADER_LEN 5
#define BUFFER_SIZE 1024
#define REQUESTOR_DEFAULT_PORT 7000

typedef struct {
    void **requestor;
    uint8_t buffer_in[BUFFER_SIZE];
    size_t buffer_in_recv;
    uint8_t buffer_out[BUFFER_SIZE];
    size_t buffer_out_send;
} ipc_od_ext_t;

typedef struct {
    uint32_t id;
    uint32_t port;
    void *socket;
} requestor_t;

static void *context = NULL;
static void *responder = NULL;
static void *broadcaster = NULL;
static void *consumer = NULL;

static requestor_t *requestor_list = NULL;
static uint8_t requestors = 0;

static ODR_t ipc_od_write_cb(OD_stream_t* stream, const void* buf, OD_size_t count, OD_size_t* countWritten);
static ODR_t ipc_od_read_cb(OD_stream_t* stream, void* buf, OD_size_t count, OD_size_t* countRead);
static requestor_t* find_requestor(uint32_t id);


void ipc_init(OD_t *od) {
    context = zmq_ctx_new();
    responder = zmq_socket(context, ZMQ_ROUTER);
    zmq_bind(responder, "tcp://*:6000");
    broadcaster = zmq_socket(context, ZMQ_PUB);
    zmq_bind(broadcaster, "tcp://*:6001");
    consumer = zmq_socket(context, ZMQ_SUB);
    zmq_setsockopt(consumer, ZMQ_SUBSCRIBE, NULL, 0);
    zmq_bind(consumer, "tcp://*:6002");

    for (size_t i=0; i < od->size; i++) {
        if (od->list[i].index < 0x4000) {
            continue;
        }

        OD_extension_t *ext = malloc(sizeof(OD_extension_t));
        if (ext == NULL) {
            log_error("malloc client od extension");
            continue;
        }

        ipc_od_ext_t *od_ext = malloc(sizeof(ipc_od_ext_t));
        if (od_ext == NULL) {
            log_error("malloc client od extension data");
            free(ext);
            continue;
        }

        od_ext->requestor = malloc(sizeof(void *) * od->list[i].subEntriesCount);
        if (od_ext->requestor == NULL) {
            log_error("malloc client od extension requestors list");
            free(od_ext);
            free(ext);
            continue;
        }

        memset(od_ext->requestor, 0, sizeof(void *) * od->list[i].subEntriesCount);
        od_ext->buffer_in_recv = 0;
        od_ext->buffer_out_send = 0;

        ext->object = od_ext;
        ext->read = ipc_od_read_cb;
        ext->write = ipc_od_write_cb;
        memset(ext->flagsPDO, 0, sizeof(uint32_t));

        od->list[i].extension = ext;
    }
}

static requestor_t* find_requestor(uint32_t id) {
    requestor_t *requestor = NULL;
    for (int i=0; i<requestors; i++) {
        if (requestor_list[i].id == id) {
            requestor = &requestor_list[i];
            break;
        }
    }
    return requestor;
}

void ipc_responder_process(CO_t* co, OD_t* od, CO_config_t *config) {
    if (!co || !od || !config) {
        log_error("ipc process null arg");
        return;
    }

    static uint32_t next_requestor_port;
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
    if (nbytes != ZMQ_HEADER_LEN) {
        log_error("zmq msg recv header error %d", errno);
        zmq_msg_close(&msg);
        return;
    } else if (nbytes != ZMQ_HEADER_LEN) {
        log_error("unexpected header len %d", nbytes);
        zmq_msg_close(&msg);
        return;
    }
    memcpy(header, zmq_msg_data(&msg), nbytes);
    uint32_t requestor_id = *(uint32_t *)&header[1]; // 0th bytes is 0

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
        case IPC_MSG_ID_REQUEST_PORT:
        {
            if (buffer_in_recv == sizeof(ipc_msg_request_port_t)) {
                requestor_t *new_requestor = NULL;

                if (requestors == 0) {
                    next_requestor_port = REQUESTOR_DEFAULT_PORT;
                    requestor_list = malloc(sizeof(requestor_t));
                    if (requestor_list) {
                        new_requestor = requestor_list;
                    }
                } else {
                    void *tmp = realloc(requestor_list, sizeof(requestor_t) * (requestors + 1));
                    if (tmp) {
                        requestor_list = tmp;
                        new_requestor = &requestor_list[requestors];
                    } else {
                        log_error("realloc reqestor failed");
                        break;
                    }
                }

                if (new_requestor) {
                    char addr[32];
                    requestors++;
                    new_requestor->id = requestor_id;
                    new_requestor->port = next_requestor_port;
                    new_requestor->socket = zmq_socket(context, ZMQ_ROUTER);
                    sprintf(addr, "tcp://*:%d", new_requestor->port);
                    zmq_bind(new_requestor->socket, addr);

                    int timeout_ms = 1;
                    zmq_setsockopt(new_requestor->socket, ZMQ_RCVTIMEO, &timeout_ms, sizeof(timeout_ms));
                    log_info("new client 0x%X at port %d", new_requestor->id, new_requestor->port);
                }

                ipc_msg_request_port_t *msg_request_port = (ipc_msg_request_port_t *)&buffer_in;
                msg_request_port->port = next_requestor_port;

                memcpy(buffer_out, msg_request_port, buffer_in_recv);
                buffer_out_send = buffer_in_recv;
            } else {
                log_error("unexpected length for new client message: %d", buffer_in_recv);
            }
            break;
        }
        case IPC_MSG_ID_REQUEST_OWNERSHIP:
        {
            if (buffer_in_recv == sizeof(ipc_msg_request_ownership_t)) {
                ipc_msg_request_ownership_t msg_request_ownership;
                memcpy(&msg_request_ownership, buffer_in, sizeof(ipc_msg_request_ownership_t));

                OD_entry_t *entry = OD_find(od, msg_request_ownership.index);
                if (entry && entry->extension && entry->extension->object) {
                    ipc_od_ext_t *od_ext = (ipc_od_ext_t *)entry->extension->object;
                    requestor_t *reqestor = find_requestor(requestor_id);
                    if (reqestor) {
                        od_ext->requestor[msg_request_ownership.subindex] = reqestor->socket;
                        log_info("add od extension cb for index 0x%X subindex 0x%X",
                                    msg_request_ownership.index, msg_request_ownership.subindex);
                        memcpy(buffer_out, buffer_in, buffer_in_recv);
                        buffer_out_send = buffer_in_recv;
                    }
                }
            } else {
                log_error("unexpected length for request ownership message: %d", buffer_in_recv);
            }
            break;
        }
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
                    //log_debug("tpdo send %d", msg_tpdo.num);
                    co->TPDO[msg_tpdo.num].sendRequest = true;
                } else {
                    log_error("invalid tpdo number %d", msg_tpdo.num);
                }
            } else {
                log_error("consumer tpdo send message size error");
            }
            break;
        }
        case IPC_MSG_ID_OD_WRITE:
        {
            if (buffer_in_recv > (int)sizeof(ipc_msg_od_t)) {
                ipc_msg_od_t msg_od;
                memcpy(&msg_od, buffer_in, sizeof(ipc_msg_od_t));
                //log_debug("od write index 0x%X subindex 0x%X", msg_od.index, msg_od.subindex);
                OD_entry_t *entry = OD_find(od, msg_od.index);
                uint8_t *data = &buffer_in[sizeof(ipc_msg_od_t)];
                size_t data_len = buffer_in_recv - sizeof(ipc_msg_od_t);
                OD_set_value(entry, msg_od.subindex, data, data_len, true);
            } else {
                log_error("consumer od write message size error");
            }
            break;
        }
        default:
        {
            log_debug("consumer unknown msg id %d", buffer_in[0]);
            break;
        }
    }
}

void ipc_free(OD_t *od) {
    for (int i=0; i < od->size; i++) {
        if (od->list[i].index < 0x4000) {
            continue;
        }
        ipc_od_ext_t *od_ext = (ipc_od_ext_t *)od->list[i].extension->object;
        if (od_ext) {
            if (od_ext->requestor) {
                free(od_ext->requestor);
            }
            free(od_ext);
            od->list[i].extension->object = NULL;
        }
    }

    if (requestor_list) {
        for (int i=0; i<requestors; i++) {
            if (requestor_list[i].socket) {
                zmq_close(requestor_list[i].socket);
            }
        }
        free(requestor_list);
    }

    zmq_close(responder);
    zmq_close(broadcaster);
    zmq_close(consumer);
    zmq_ctx_term(context);
}

static ODR_t ipc_od_read_cb(OD_stream_t* stream, void* buf, OD_size_t count, OD_size_t* countRead) {
    if (!stream || !buf || !countRead || !stream->object) {
        log_error("null arg error");
        return ODR_DEV_INCOMPAT;
    }

    ipc_od_ext_t *od_ext = (ipc_od_ext_t *)stream->object;
    if ((od_ext->requestor == NULL) || (od_ext->requestor[stream->subIndex] == NULL)) {
        return OD_readOriginal(stream, buf, count, countRead);
    }

    if (stream->dataOffset == 0U) {
        ipc_msg_od_t msg_od = {
            .id = IPC_MSG_ID_OD_READ,
            .index = stream->index,
            .subindex = stream->subIndex,
        };

        void *requestor = od_ext->requestor[stream->subIndex];
        log_debug("od read callback for index 0x%X subindex 0x%X", msg_od.index, msg_od.subindex);
        zmq_send(requestor, &msg_od, sizeof(ipc_msg_od_t), 0);

        int buffer_in_recv = zmq_recv(requestor, od_ext->buffer_in, BUFFER_SIZE, 0);
        if (buffer_in_recv == -1) {
            log_error("od read callback for index 0x%X subindex 0x%X recv timeout",
                      msg_od.index, msg_od.subindex);
            return ODR_DEV_INCOMPAT;
        } else if ((buffer_in_recv != (int)od_ext->buffer_out_send) || (od_ext->buffer_in[0] != IPC_MSG_ID_OD_READ)) {
            log_error("od read cb error");
            return ODR_DEV_INCOMPAT;
        }

        stream->dataLength = buffer_in_recv - sizeof(ipc_msg_od_t);
    }

    return od_ext_read_data(stream, buf, count, countRead, &od_ext->buffer_in[sizeof(ipc_msg_od_t)], stream->dataLength);
}

static ODR_t ipc_od_write_cb(OD_stream_t* stream, const void* buf, OD_size_t count, OD_size_t* countWritten) {
    if (!stream || !buf || !stream->object) {
        log_error("null arg error");
        return ODR_DEV_INCOMPAT;
    }

    ODR_t r;
    ipc_od_ext_t *od_ext = (ipc_od_ext_t *)stream->object;

    if ((od_ext->requestor == NULL) || (od_ext->requestor[stream->subIndex] == NULL)) {
        r = OD_writeOriginal(stream, buf, count, countWritten);
        if (r == ODR_PARTIAL) {
            uint32_t offset = sizeof(ipc_msg_od_t) + stream->dataOffset - count;
            memcpy(&od_ext->buffer_in[offset], buf, *countWritten);
        } else if (r == ODR_OK) {
            if (stream->dataLength <= *countWritten) { // data fits in one segment
                memcpy(&od_ext->buffer_in[sizeof(ipc_msg_od_t)], buf, *countWritten);
            } else { // last segment
                uint32_t offset = sizeof(ipc_msg_od_t) + stream->dataOffset - count;
                memcpy(&od_ext->buffer_in[offset], buf, *countWritten);
            }
        }
    } else {
        uint8_t *data = &od_ext->buffer_in[sizeof(ipc_msg_od_t)];
        size_t data_len = od_ext->buffer_in_recv - sizeof(ipc_msg_od_t);
        r = od_ext_write_data(stream, buf, count, countWritten, data, BUFFER_SIZE, &data_len);
    }

    if (r == ODR_OK) {
        void *requestor = od_ext->requestor[stream->subIndex];
        ipc_msg_od_t msg_od = {
            .id = IPC_MSG_ID_OD_WRITE,
            .index = stream->index,
            .subindex = stream->subIndex,
        };
        memcpy(od_ext->buffer_out, &msg_od, sizeof(ipc_msg_od_t));

        int buffer_out_send = sizeof(ipc_msg_od_t) + stream->dataLength;
        log_debug("od write callback for index 0x%X subindex 0x%X", msg_od.index, msg_od.subindex);

        if (requestor) {
            zmq_send(requestor, &od_ext->buffer_out, buffer_out_send, 0);

            od_ext->buffer_in_recv = zmq_recv(requestor, od_ext->buffer_in, BUFFER_SIZE, 0);
            if (((int)od_ext->buffer_in_recv != buffer_out_send) || (od_ext->buffer_in[0] != IPC_MSG_ID_OD_WRITE)) {
                return ODR_DEV_INCOMPAT;
            }
        }

        zmq_send(broadcaster, &od_ext->buffer_in, od_ext->buffer_in_recv, 0);
    }

    return r;
}
