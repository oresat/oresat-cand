#include "ipc_consume.h"
#include "CANopen.h"
#include "ipc_broadcast.h"
#include "ipc_msg.h"
#include "logger.h"
#include "system.h"
#include <libgen.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <zmq.h>

static void *consumer = NULL;

static void ipc_consume_emcy_send(uint8_t *buffer_in, uint32_t buffer_in_recv, CO_t *co);
static void ipc_consume_tpdo_send(uint8_t *buffer_in, uint32_t buffer_in_recv, CO_t *co, CO_config_t *base_config,
                                  CO_config_t *config);
static void ipc_consume_sync_send(CO_t *co, CO_config_t *config);
static void ipc_consume_od_write(uint8_t *buffer_in, uint32_t buffer_in_recv, OD_t *od);
static void ipc_consume_config(uint8_t *buffer_in, uint32_t buffer_in_recv, char *od_config_path, bool *reset);

int ipc_consume_init(void *context) {
    if (!context) {
        return -EINVAL;
    }

    consumer = zmq_socket(context, ZMQ_SUB);
    zmq_setsockopt(consumer, ZMQ_SUBSCRIBE, NULL, 0);
    zmq_bind(consumer, "tcp://*:6002");

    return 0;
}

void ipc_consume_process(CO_t *co, OD_t *od, CO_config_t *base_config, CO_config_t *config, char *od_config_path,
                         bool *reset) {
    if (!co || !od || !base_config || !config || !od_config_path) {
        log_error("null args");
        return;
    }

    static uint8_t buffer_in[IPC_MSG_MAX_LEN];
    int nbytes = zmq_recv(consumer, buffer_in, IPC_MSG_MAX_LEN, 0);
    if (nbytes <= (int)sizeof(ipc_header_t)) {
        return;
    }

    uint32_t buffer_in_recv = nbytes;
    switch (buffer_in[1]) {
    case IPC_MSG_ID_EMCY_SEND:
        ipc_consume_emcy_send(buffer_in, buffer_in_recv, co);
        break;
    case IPC_MSG_ID_TPDO_SEND:
        ipc_consume_tpdo_send(buffer_in, buffer_in_recv, co, base_config, config);
        break;
    case IPC_MSG_ID_OD_WRITE:
        ipc_consume_od_write(buffer_in, buffer_in_recv, od);
        break;
    case IPC_MSG_ID_SYNC_SEND:
        ipc_consume_sync_send(co, config);
        break;
    case IPC_MSG_ID_CONFIG:
        ipc_consume_config(buffer_in, buffer_in_recv, od_config_path, reset);
        break;
    default:
        break;
    }
}

void ipc_consume_free(void) {
    if (consumer) {
        zmq_close(consumer);
        consumer = NULL;
    }
}

static void ipc_consume_emcy_send(uint8_t *buffer_in, uint32_t buffer_in_recv, CO_t *co) {
    if (buffer_in_recv != sizeof(ipc_msg_emcy_send_t)) {
        log_error("msg len mismatch; got %d expect %d", buffer_in_recv, sizeof(ipc_msg_emcy_send_t));
        return;
    }
    ipc_msg_emcy_send_t *msg_emcy_send = (ipc_msg_emcy_send_t *)buffer_in;
    log_info("emcy send with code 0x%X info 0x%X", msg_emcy_send->code, msg_emcy_send->info);
    CO_errorReport(co->em, CO_EM_GENERIC_ERROR, msg_emcy_send->code, msg_emcy_send->info);
}

static void ipc_consume_tpdo_send(uint8_t *buffer_in, uint32_t buffer_in_recv, CO_t *co, CO_config_t *base_config,
                                  CO_config_t *config) {
    if (buffer_in_recv != sizeof(ipc_msg_tpdo_send_t)) {
        log_error("msg len mismatch; got %d expect %d", buffer_in_recv, sizeof(ipc_msg_tpdo_send_t));
        return;
    }
    ipc_msg_tpdo_send_t *msg_tpdo_send = (ipc_msg_tpdo_send_t *)buffer_in;
    if (msg_tpdo_send->number < config->CNT_TPDO) {
        log_debug("tpdo send %d", msg_tpdo_send->number);
        uint8_t tpdos_offset = 0;
        if (base_config->CNT_TPDO != config->CNT_TPDO) {
            tpdos_offset = base_config->CNT_TPDO; // add the common base tpdos to num
        }
        co->TPDO[msg_tpdo_send->number + tpdos_offset].sendRequest = true;
    } else {
        log_error("invalid tpdo number %d", msg_tpdo_send->number);
    }
}

static void ipc_consume_od_write(uint8_t *buffer_in, uint32_t buffer_in_recv, OD_t *od) {
    if ((buffer_in_recv < IPC_MSG_OD_MIN_LEN) || (buffer_in_recv > sizeof(ipc_msg_od_t))) {
        log_error("msg len mismatch; got %d, expect between %d to %d",
                   buffer_in_recv, IPC_MSG_OD_MIN_LEN, sizeof(ipc_msg_od_t));
        return;
    }
    ipc_msg_od_t *msg_od = (ipc_msg_od_t *)&buffer_in;
    log_debug("od write index 0x%X subindex 0x%X", msg_od->index, msg_od->subindex);
    OD_entry_t *entry = OD_find(od, msg_od->index);
    OD_set_value(entry, msg_od->subindex, msg_od->buffer.data, msg_od->buffer.len, ipc_clients_count() <= 1);
}

static void ipc_consume_sync_send(CO_t *co, CO_config_t *config) {
    if (config->CNT_SYNC) {
        log_debug("sync send");
        CO_SYNCsend(co->SYNC);
    } else {
        log_error("sync send not valid for node");
    }
}

static void ipc_consume_config(uint8_t *buffer_in, uint32_t buffer_in_recv, char *od_config_path, bool *reset) {
    if ((buffer_in_recv < IPC_MSG_FILE_MIN_LEN) || (buffer_in_recv > sizeof(ipc_msg_file_t))) {
        log_error("msg len mismatch; got %d, expect between %d to %d",
                   buffer_in_recv, IPC_MSG_FILE_MIN_LEN, sizeof(ipc_msg_file_t));
        return;
    }

    ipc_msg_file_t *msg_file = (ipc_msg_file_t *)&buffer_in;
    msg_file->path.data[msg_file->path.len] = '\0';
    if (is_file(msg_file->path.data)) {
        int r;
        log_debug("od config: local %s vs apps %s", od_config_path, msg_file->path.data);
        if (check_file_crc32_match(od_config_path, msg_file->path.data)) {
            log_info("local od config matches apps");
        } else {
            log_info("local od config does not match apps");
            char *path_copy = strdup(od_config_path);
            mkdir_path(dirname(path_copy), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            free(path_copy);
            r = copy_file(msg_file->path.data, od_config_path);
            if (r < 0) {
                log_error("failed to update local od config to apps");
            } else {
                log_info("updated local od config to apps");
                log_info("resetting app to load new config");
                *reset = true;
            }
        }
    } else {
        log_error("file %s not found", msg_file->path.data);
    }
}
