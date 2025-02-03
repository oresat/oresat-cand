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

#define BUFFER_SIZE 10240

static void *context = NULL;
static void *responder = NULL;

void ipc_init(void) {
    context = zmq_ctx_new();
    responder = zmq_socket(context, ZMQ_REP);
    zmq_bind(responder, "tcp://*:"IPC_PORT);
}

void ipc_process(CO_t* co, OD_t* od, CO_config_t *config) {
    if (!co || !od || !config) {
        log_error("ipc process null arg");
        return;
    }

    static char buffer_in[BUFFER_SIZE];
    static int buffer_in_recv = 0;
    static char buffer_out[BUFFER_SIZE];
    static int buffer_out_send = 0;

    buffer_in_recv = zmq_recv(responder, buffer_in, BUFFER_SIZE, ZMQ_DONTWAIT);
    if (buffer_in_recv == -1) {
        return;
    }

    buffer_out[0] = IPC_MSG_ID_ERROR_LENGTH;
    buffer_out_send = 1;

    switch (buffer_in[0]) {
        case IPC_MSG_ID_SEND_EMCY:
        {
            if (buffer_in_recv == sizeof(ipc_msg_emcy_t)) {
                ipc_msg_emcy_t msg_emcy;
                memcpy(&msg_emcy, buffer_in, sizeof(ipc_msg_emcy_t));
                log_debug("emcy send with code 0x%X info 0x%X", msg_emcy.code, msg_emcy.info);
                CO_errorReport(co->em, CO_EM_GENERIC_ERROR, msg_emcy.code, msg_emcy.info);
                memcpy(buffer_out, buffer_in, buffer_in_recv);
                buffer_out_send = buffer_in_recv;
            }
            break;
        }
        case IPC_MSG_ID_SEND_TPDO:
        {
            if (buffer_in_recv == 2) {
                uint8_t num = buffer_in[1];
                if (num < config->CNT_TPDO) {
                    log_debug("tpdo send %d", num);
                    co->TPDO[num].sendRequest = true;
                    memcpy(buffer_out, buffer_in, buffer_in_recv);
                    buffer_out_send = buffer_in_recv;
                } else {
                    log_error("invalid tpdo number %d", num);
                    buffer_out[0] = IPC_MSG_ID_ERROR_TPDO_NUM;
                    buffer_out_send = 1;
                }
            }
            break;
        }
        case IPC_MSG_ID_OD_READ:
        {
            if (buffer_in_recv == sizeof(ipc_msg_od_t)) {
                ipc_msg_od_t msg_od;
                memcpy(&msg_od, buffer_in, sizeof(ipc_msg_od_t));
                log_debug("od read index 0x%X subindex 0x%X", msg_od.index, msg_od.subindex);
                OD_entry_t *entry = OD_find(od, msg_od.index);
                OD_IO_t io = { 0 };
                ODR_t r = OD_getSub(entry, msg_od.subindex, &io, false);
                if (r == ODR_OK) {
                    memcpy(buffer_out, buffer_in, buffer_in_recv);
                    OD_get_value(entry, msg_od.subindex, &buffer_out[buffer_in_recv], 4, false);
                    memcpy(&buffer_out[buffer_in_recv], io.stream.dataOrig, io.stream.dataLength);
                    buffer_out_send = buffer_in_recv + io.stream.dataLength;
                } else {
                    buffer_out[0] = IPC_MSG_ID_ERROR_OD_ABORT;
                    CO_SDO_abortCode_t ac = OD_getSDOabCode(r);
                    memcpy(&buffer_out[1], &ac, 4);
                    buffer_out_send = 5;
                }
            }
            break;
        }
        case IPC_MSG_ID_OD_WRITE:
        {
            if (buffer_in_recv > (int)sizeof(ipc_msg_od_t)) {
                ipc_msg_od_t msg_od;
                memcpy(&msg_od, buffer_in, sizeof(ipc_msg_od_t));
                log_debug("od write index 0x%X subindex 0x%X", msg_od.index, msg_od.subindex);
                OD_entry_t *entry = OD_find(od, msg_od.index);
                ODR_t r = OD_set_value(entry, msg_od.subindex, &buffer_in[sizeof(ipc_msg_od_t)], buffer_in_recv - sizeof(ipc_msg_od_t), false);
                if (r == ODR_OK) {
                    memcpy(buffer_out, buffer_in, buffer_in_recv);
                    buffer_out_send = buffer_in_recv;
                } else {
                    buffer_out[0] = IPC_MSG_ID_ERROR_OD_ABORT;
                    CO_SDO_abortCode_t ac = OD_getSDOabCode(r);
                    memcpy(&buffer_out[1], &ac, 4);
                    buffer_out_send = 5;
                }
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
                        buffer_out[0] = IPC_MSG_ID_SDO_READ;
                        buffer_out_send = 1;
                        free(data);
                    } else {
                        memcpy(buffer_out, buffer_in, buffer_in_recv);
                        memcpy(&buffer_out[buffer_in_recv], data, data_len);
                        buffer_out_send = buffer_in_recv + data_len;
                        free(data);
                    }
                } else {
                    buffer_out[0] = IPC_MSG_ID_ERROR_SDO_ABORT;
                    memcpy(&buffer_out[1], &ac, 4);
                    buffer_out_send = 5;
                }
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
                    ac = sdo_write(co->SDOclient, msg_sdo.node_id, msg_sdo.index, msg_sdo.subindex, &buffer_in[sizeof(ipc_msg_od_t)], buffer_in_recv - 6);
                }
                if (ac == 0) {
                    memcpy(buffer_out, buffer_in, buffer_in_recv);
                    buffer_out_send = buffer_in_recv;
                } else {
                    buffer_out[0] = IPC_MSG_ID_ERROR_SDO_ABORT;
                    memcpy(&buffer_out[1], &ac, 4);
                    buffer_out_send = 5;
                }
            }
            break;
        }
        default:
        {
            log_debug("unknwon msg id %d", buffer_in[0]);
            buffer_out[0] = IPC_MSG_ID_ERROR_UNKNOWN_ID;
            buffer_out_send = 1;
            break;
        }
    }

    zmq_send(responder, buffer_out, buffer_out_send, 0);
}

void ipc_free(void) {
    zmq_close(responder);
    zmq_ctx_term(context);
}
