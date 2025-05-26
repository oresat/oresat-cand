#include "ipc_broadcaster.h"
#include "CANopen.h"
#include "ipc_msg.h"
#include "logger.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <zmq.h>

#define LOG_BROADCASTER "broadcaster: "

#define CAN_BUS_NOT_FOUND 0
#define CAN_BUS_DOWN      1
#define CAN_BUS_UP        2

static void *broadcaster = NULL;
static void *monitor = NULL;
static uint8_t clients = 0;

static ODR_t ipc_broadcaster_data(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten);

static OD_extension_t ext = {
    .object = NULL,
    .read = OD_readOriginal,
    .write = ipc_broadcaster_data,
};

int ipc_broadcaster_init(void *context, OD_t *od) {
    if (!context || !od) {
        return -EINVAL;
    }

    broadcaster = zmq_socket(context, ZMQ_PUB);
    zmq_bind(broadcaster, "tcp://*:6001");

    zmq_socket_monitor(broadcaster, "inproc://monitor", ZMQ_EVENT_ACCEPTED | ZMQ_EVENT_DISCONNECTED);
    monitor = zmq_socket(context, ZMQ_PAIR);
    zmq_connect(monitor, "inproc://monitor");

    for (int i = 0; i < od->size; i++) {
        if (od->list[i].index >= 0x4000) {
            OD_extension_init(&od->list[i], &ext);
        }
    }

    return 0;
}

void ipc_broadcaster_process(void) {
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

void ipc_broadcaster_free(void) {
    zmq_close(broadcaster);
    zmq_close(monitor);
}

uint8_t ipc_clients_count(void) {
    return clients;
}

static ODR_t ipc_broadcaster_data(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten) {
    ODR_t r = OD_writeOriginal(stream, buf, count, countWritten);
    if ((r == ODR_OK) && (stream->dataOrig != NULL)) {
        ipc_msg_od_t msg_od = {
            .id = IPC_MSG_ID_OD_WRITE,
            .index = stream->index,
            .subindex = stream->subIndex,
        };
        int length = sizeof(ipc_msg_od_t) + sizeof(stream->dataLength) + 1;
        uint8_t *buf = malloc(length);
        if (buf == NULL) {
            log_error(LOG_BROADCASTER "malloc error");
        } else {
            buf[0] = IPC_MSG_VERSION;
            memcpy(&buf[1], &msg_od, sizeof(ipc_msg_od_t));
            uint8_t *data_orig = (uint8_t *)stream->dataOrig;
            memcpy(&buf[1 + sizeof(ipc_msg_od_t)], data_orig, stream->dataLength);
            zmq_send(broadcaster, buf, length, 0);
            log_debug(LOG_BROADCASTER "od write index 0x%X subindex 0x%X", msg_od.index, msg_od.subindex);
            free(buf);
        }
    }
    return r;
}

void ipc_broadcaster_hb(uint8_t node_id, uint8_t state) {
    ipc_msg_hb_t msg_hb = {
        .id = IPC_MSG_ID_HB_RECV,
        .node_id = node_id,
        .state = state,
    };
    size_t size = 1 + sizeof(ipc_msg_hb_t);
    uint8_t buf[size];
    buf[0] = IPC_MSG_VERSION;
    memcpy(&buf[1], &msg_hb, sizeof(ipc_msg_hb_t));
    zmq_send(broadcaster, &buf, size, 0);
}

void ipc_broadcaster_emcy(uint8_t node_id, uint16_t code, uint32_t info) {
    ipc_msg_emcy_recv_t msg_emcy_recv = {
        .id = IPC_MSG_ID_EMCY_RECV,
        .node_id = node_id,
        .code = code,
        .info = info,
    };
    size_t size = 1 + sizeof(ipc_msg_emcy_recv_t);
    uint8_t buf[size];
    buf[0] = IPC_MSG_VERSION;
    memcpy(&buf[1], &msg_emcy_recv, sizeof(ipc_msg_emcy_recv_t));
    zmq_send(broadcaster, &buf, size, 0);
}

static void ipc_broadcaster_status(uint8_t status) {
    uint8_t data[] = {IPC_MSG_VERSION, IPC_MSG_ID_BUS_STATUS, status};
    zmq_send(broadcaster, data, 3, 0);
}

void ipc_broadcaster_bus_status(CO_t *co) {
    if (!co || !co->CANmodule || !co->CANmodule->CANinterfaces) {
        ipc_broadcaster_status(CAN_BUS_NOT_FOUND);
        return;
    }

    char file_path[50];
    sprintf(file_path, "/sys/class/net/%s/carrier", co->CANmodule->CANinterfaces[0].ifName);
    FILE *fp = fopen(file_path, "r");
    if (fp) {
        char up;
        fread(&up, 1, 1, fp);
        fclose(fp);
        if (up == '1') {
            ipc_broadcaster_status(CAN_BUS_UP);
        } else {
            ipc_broadcaster_status(CAN_BUS_DOWN);
        }
    } else {
        ipc_broadcaster_status(CAN_BUS_NOT_FOUND);
    }
}
