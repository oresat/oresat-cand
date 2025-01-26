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
#include "system.h"
#include "logger.h"
#include "ipc.h"

#define PORT "5555"
#define BUFFER_SIZE 10240

#define MSG_ID_EMCY_SEND        0x00
#define MSG_ID_TPDO_SEND        0x01
#define MSG_ID_OD_READ          0x02
#define MSG_ID_OD_WRITE         0x03
#define MSG_ID_SDO_READ         0x04
#define MSG_ID_SDO_WRITE        0x05
#define MSG_ID_ERROR_UNKNOWN_ID 0x80
#define MSG_ID_ERROR_LENGTH     0x81
#define MSG_ID_ERROR_TPDO_NUM   0x82
#define MSG_ID_ERROR_OD_ABORT   0x83
#define MSG_ID_ERROR_SDO_ABORT  0x84

typedef struct __attribute((packed)) {
    uint8_t id;
    uint16_t index;
    uint8_t subindex;
    uint8_t dtype;
} msg_id_od_t;

typedef struct __attribute((packed)) {
    uint8_t id;
    uint8_t node_id;
    uint16_t index;
    uint8_t subindex;
    uint8_t dtype;
} msg_id_sdo_t;

static pthread_t thread_id;
static bool running = false;

static void* ipc_args_thread(void *arg);

void ipc_init(ipc_args_t *ipc_args) {
    if (ipc_args) {
        pthread_create(&thread_id, NULL, ipc_args_thread, ipc_args);
    }
}

static void* ipc_args_thread(void *arg) {
    ipc_args_t *ipc_args = (ipc_args_t *)arg;
    CO_t* CO = ipc_args->co;
    OD_t* od = ipc_args->od;
    CO_config_t *config = ipc_args->config;
    void *context = zmq_ctx_new();
    void *responder = zmq_socket(context, ZMQ_REP);
    zmq_bind(responder, "tcp://*:"PORT);

    char buffer_in[BUFFER_SIZE];
    int buffer_in_recv = 0;
    char buffer_out[BUFFER_SIZE];
    int buffer_out_send = 0;

    running = true;
    while(running) {
        buffer_in_recv = zmq_recv(responder, buffer_in, BUFFER_SIZE, ZMQ_DONTWAIT);
        if (buffer_in_recv == -1) {
            sleep_ms(100);
            continue;
        }

        buffer_out[0] = MSG_ID_ERROR_LENGTH;
        buffer_out_send = 1;

        switch (buffer_in[0]) {
            case MSG_ID_EMCY_SEND:
            {
                if (buffer_in_recv == 5) {
                    uint32_t code;
                    memcpy(&code, &buffer_in[1], sizeof(code));
                    log_debug("emcy send 0x%X", code);
                    CO_errorReport(CO->em, CO_EM_GENERIC_ERROR,code, 0);
                    memcpy(buffer_out, buffer_in, buffer_in_recv);
                    buffer_out_send = buffer_in_recv;
                }
                break;
            }
            case MSG_ID_TPDO_SEND:
            {
                if (buffer_in_recv == 2) {
                    uint8_t num  = buffer_in[1];
                    if (num < config->CNT_TPDO) {
                        log_debug("tpdo send %d", num);
                        CO_TPDOsendRequest(&CO->TPDO[num]);
                        memcpy(buffer_out, buffer_in, buffer_in_recv);
                        buffer_out_send = buffer_in_recv;
                    } else {
                        buffer_out[0] = MSG_ID_ERROR_TPDO_NUM;
                        buffer_out_send = 1;
                    }
                }
                break;
            }
            case MSG_ID_OD_READ:
            {
                if (buffer_in_recv == sizeof(msg_id_od_t)) {
                    msg_id_od_t msg_id_od;
                    memcpy(&msg_id_od, buffer_in, sizeof(msg_id_od_t));
                    log_debug("od read index 0x%X subindex 0x%X", msg_id_od.index, msg_id_od.subindex);
                    OD_entry_t *entry = OD_find(od, msg_id_od.index);
                    OD_IO_t io = { 0 };
                    ODR_t r = OD_getSub(entry, msg_id_od.subindex, &io, false);
                    if (r == ODR_OK) {
                        memcpy(buffer_out, buffer_in, buffer_in_recv);
                        OD_get_value(entry, msg_id_od.subindex, &buffer_out[buffer_in_recv], 4, false);
                        memcpy(&buffer_out[buffer_in_recv], io.stream.dataOrig, io.stream.dataLength);
                        buffer_out_send = buffer_in_recv + io.stream.dataLength;
                    } else {
                        buffer_out[0] = MSG_ID_ERROR_OD_ABORT;
                        CO_SDO_abortCode_t ac = OD_getSDOabCode(r);
                        memcpy(&buffer_out[1], &ac, 4);
                        buffer_out_send = 5;
                    }
                }
                break;
            }
            case MSG_ID_OD_WRITE:
            {
                if (buffer_in_recv > (int)sizeof(msg_id_od_t)) {
                    msg_id_od_t msg_id_od;
                    memcpy(&msg_id_od, buffer_in, sizeof(msg_id_od_t));
                    log_debug("od write index 0x%X subindex 0x%X", msg_id_od.index, msg_id_od.subindex);
                    OD_entry_t *entry = OD_find(od, msg_id_od.index);
                    ODR_t r = OD_set_value(entry, msg_id_od.subindex, &buffer_in[sizeof(msg_id_od_t)], buffer_in_recv - sizeof(msg_id_od_t), false);
                    if (r == ODR_OK) {
                        memcpy(buffer_out, buffer_in, buffer_in_recv);
                        buffer_out_send = buffer_in_recv;
                    } else {
                        buffer_out[0] = MSG_ID_ERROR_OD_ABORT;
                        CO_SDO_abortCode_t ac = OD_getSDOabCode(r);
                        memcpy(&buffer_out[1], &ac, 4);
                        buffer_out_send = 5;
                    }
                }
                break;
            }
            case MSG_ID_SDO_READ:
            {
                if (buffer_in_recv == sizeof(msg_id_sdo_t)) {
                    msg_id_sdo_t msg_id_sdo;
                    memcpy(&msg_id_sdo, buffer_in, sizeof(msg_id_sdo_t));
                    log_debug("sdo read index 0x%X subindex 0x%X", msg_id_sdo.index, msg_id_sdo.subindex);
                    void *data = NULL;
                    size_t data_len = 0;
                    CO_SDO_abortCode_t ac = -1;
                    if (CO && CO->SDOclient) {
                        ac = sdo_read_dynamic(CO->SDOclient, msg_id_sdo.node_id, msg_id_sdo.index, msg_id_sdo.subindex, &data, &data_len);
                    }
                    if (ac == 0) {
                        if (data == NULL) {
                            memcpy(buffer_out, buffer_in, buffer_in_recv);
                            buffer_out_send = buffer_in_recv;
                        } else if ((buffer_in_recv + data_len) > BUFFER_SIZE) {
                            buffer_out[0] = MSG_ID_SDO_READ;
                            buffer_out_send = 1;
                            free(data);
                        } else {
                            memcpy(buffer_out, buffer_in, buffer_in_recv);
                            memcpy(&buffer_out[buffer_in_recv], data, data_len);
                            buffer_out_send = buffer_in_recv + data_len;
                            free(data);
                        }
                    } else {
                        buffer_out[0] = MSG_ID_ERROR_SDO_ABORT;
                        memcpy(&buffer_out[1], &ac, 4);
                        buffer_out_send = 5;
                    }
                }
                break;
            }
            case MSG_ID_SDO_WRITE:
            {
                if (buffer_in_recv > (int)sizeof(msg_id_sdo_t)) {
                    msg_id_sdo_t msg_id_sdo;
                    memcpy(&msg_id_sdo, buffer_in, sizeof(msg_id_sdo_t));
                    log_debug("sdo write index 0x%X subindex 0x%X", msg_id_sdo.index, msg_id_sdo.subindex);
                    CO_SDO_abortCode_t ac = -1;
                    if (CO && CO->SDOclient) {
                        ac = sdo_write(CO->SDOclient, msg_id_sdo.node_id, msg_id_sdo.index, msg_id_sdo.subindex, &buffer_in[sizeof(msg_id_od_t)], buffer_in_recv - 6);
                    }
                    if (ac == 0) {
                        memcpy(buffer_out, buffer_in, buffer_in_recv);
                        buffer_out_send = buffer_in_recv;
                    } else {
                        buffer_out[0] = MSG_ID_ERROR_SDO_ABORT;
                        memcpy(&buffer_out[1], &ac, 4);
                        buffer_out_send = 5;
                    }
                }
                break;
            }
            default:
            {
                log_debug("unknwon msg id %d", buffer_in[0]);
                buffer_out[0] = MSG_ID_ERROR_UNKNOWN_ID;
                buffer_out_send = 1;
                break;
            }
        }

        zmq_send(responder, buffer_out, buffer_out_send, 0);
    }

    zmq_close(responder);
    zmq_ctx_term(context);
    return NULL;
}

void ipc_free(void) {
    running = false;
    pthread_join(thread_id, NULL);
}
