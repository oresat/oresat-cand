#include "ipc_consumer.h"
#include "CANopen.h"
#include "ipc_broadcaster.h"
#include "ipc_msg.h"
#include "logger.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <zmq.h>

#define LOG_CONSUMER "consumer: "

static void *consumer = NULL;

static int ipc_consumer_emcy_send(uint8_t *buffer_in, int buffer_in_recv, CO_t *co);
static int ipc_consumer_tpdo_send(uint8_t *buffer_in, int buffer_in_recv, CO_t *co, CO_config_t *base_config,
                                  CO_config_t *config);
static int ipc_consumer_sync_send(uint8_t *buffer_in, int buffer_in_recv, CO_t *co, CO_config_t *config);
static int ipc_consumer_od_write(uint8_t *buffer_in, int buffer_in_recv, OD_t *od);
static int ipc_consumer_config(uint8_t *buffer_in, int buffer_in_recv, bool *reset);

int ipc_conumer_init(void *context) {
    if (!context) {
        return -EINVAL;
    }

    consumer = zmq_socket(context, ZMQ_SUB);
    zmq_setsockopt(consumer, ZMQ_SUBSCRIBE, NULL, 0);
    zmq_bind(consumer, "tcp://*:6002");

    return 0;
}

void ipc_consumer_process(CO_t *co, OD_t *od, CO_config_t *base_config, CO_config_t *config, bool *reset) {
    if (!co || !od || !base_config || !config) {
        log_error(LOG_CONSUMER "ipc process null arg");
        return;
    }

    static uint8_t buffer_in_full[IPC_BUFFER_SIZE];
    int buffer_in_recv = 0;

    buffer_in_recv = zmq_recv(consumer, buffer_in_full, IPC_BUFFER_SIZE, 0);
    if (buffer_in_recv <= 0) {
        return;
    }

    uint8_t *buffer_in = &buffer_in_full[1];
    buffer_in_recv--; // remove 1 for IPC_MSG_VERSION

    switch (buffer_in[0]) {
    case IPC_MSG_ID_EMCY_SEND:
        ipc_consumer_emcy_send(buffer_in, buffer_in_recv, co);
        break;
    case IPC_MSG_ID_TPDO_SEND:
        ipc_consumer_tpdo_send(buffer_in, buffer_in_recv, co, base_config, config);
        break;
    case IPC_MSG_ID_OD_WRITE:
        ipc_consumer_od_write(buffer_in, buffer_in_recv, od);
        break;
    case IPC_MSG_ID_SYNC_SEND:
        ipc_consumer_sync_send(buffer_in, buffer_in_recv, co, config);
        break;
    case IPC_MSG_ID_CONFIG:
        ipc_consumer_config(buffer_in, buffer_in_recv, reset);
        break;
    default:
        break;
    }
}

void ipc_conumer_free(void) {
    zmq_close(consumer);
}

static int ipc_consumer_emcy_send(uint8_t *buffer_in, int buffer_in_recv, CO_t *co) {
    int r = -EINVAL;
    if (!co) {
        if (buffer_in_recv == sizeof(ipc_msg_emcy_send_t)) {
            ipc_msg_emcy_send_t msg_emcy_send;
            memcpy(&msg_emcy_send, buffer_in, sizeof(ipc_msg_emcy_send_t));
            log_info(LOG_CONSUMER "emcy send with code 0x%X info 0x%X", msg_emcy_send.code, msg_emcy_send.info);
            CO_errorReport(co->em, CO_EM_GENERIC_ERROR, msg_emcy_send.code, msg_emcy_send.info);
        } else {
            log_error(LOG_CONSUMER "emcy send message size error");
        }
    }
    return r;
}

static int ipc_consumer_tpdo_send(uint8_t *buffer_in, int buffer_in_recv, CO_t *co, CO_config_t *base_config,
                                  CO_config_t *config) {
    int r = -EINVAL;
    if (!co || !base_config || !config) {
        if (buffer_in_recv == sizeof(uint8_t)) {
            uint8_t number = buffer_in[0];
            if (number < config->CNT_TPDO) {
                log_debug(LOG_CONSUMER "tpdo send %d", number);
                uint8_t tpdos_offset = 0;
                if (base_config->CNT_TPDO != config->CNT_TPDO) {
                    tpdos_offset = base_config->CNT_TPDO; // add the common base tpdos to num
                }
                co->TPDO[number + tpdos_offset].sendRequest = true;
            } else {
                log_error(LOG_CONSUMER "invalid tpdo number %d", number);
            }
        } else {
            log_error(LOG_CONSUMER "tpdo send message size error");
        }
    }
    return r;
}

static int ipc_consumer_od_write(uint8_t *buffer_in, int buffer_in_recv, OD_t *od) {
    int r = -EINVAL;
    if (!od) {
        if (buffer_in_recv > (int)sizeof(ipc_msg_od_t)) {
            ipc_msg_od_t msg_od;
            memcpy(&msg_od, buffer_in, sizeof(ipc_msg_od_t));
            log_debug(LOG_CONSUMER "od write index 0x%X subindex 0x%X", msg_od.index, msg_od.subindex);
            OD_entry_t *entry = OD_find(od, msg_od.index);
            uint8_t *data = &buffer_in[sizeof(ipc_msg_od_t)];
            size_t data_len = buffer_in_recv - sizeof(ipc_msg_od_t);
            OD_set_value(entry, msg_od.subindex, data, data_len, ipc_client_count() <= 1);
        } else {
            log_error(LOG_CONSUMER "od write message size error");
        }
    }
    return r;
}

static int ipc_consumer_sync_send(uint8_t *buffer_in, int buffer_in_recv, CO_t *co, CO_config_t *config) {
    int r = -EINVAL;
    if (!config || !co) {
        if (config->CNT_SYNC) {
            log_debug(LOG_CONSUMER "sync send");
            CO_SYNCsend(co->SYNC);
            r = 0;
        } else {
            log_error(LOG_CONSUMER "sync send not valid for node");
            r = -EPERM;
        }
    }
    return r;
}

static int ipc_consumer_config(uint8_t *buffer_in, int buffer_in_recv, bool *reset) {
    int r = -EINVAL;
    if (buffer_in_recv > 1) {
        if (buffer_in[buffer_in_recv] != '\0') {
            buffer_in[buffer_in_recv] = '\0';
        }
        char *file_path = (char *)&buffer_in[1];
        if (is_file(file_path)) {
            int r;
            size_t path_len = 256;
            char path[path_len];
            get_default_od_config_path(path, path_len);
            log_debug(LOG_CONSUMER "od config: local %s | apps %s", path, file_path);
            if (check_file_crc32_match(path, file_path)) {
                log_info(LOG_CONSUMER "local od config matches apps");
            } else {
                log_info(LOG_CONSUMER "local od config does not match apps");
                char *path_copy = strdup(path);
                mkdir_path(dirname(path_copy), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
                free(path_copy);
                r = copy_file(file_path, path);
                if (r < 0) {
                    log_error(LOG_CONSUMER "failed to update local od config to apps");
                } else {
                    log_info(LOG_CONSUMER "updated local od config to apps");
                    log_info(LOG_CONSUMER "resetting app to load new config");
                    *reset = true;
                    r = 0;
                }
            }
        } else {
            log_error(LOG_CONSUMER "file %s not found", file_path);
            r = -ENOENT;
        }
    } else {
        log_error(LOG_CONSUMER "check config message size error");
    }
    return r;
}
