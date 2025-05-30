#include "ipc_broadcast.h"
#include "CANopen.h"
#include "CO_ODinterface.h"
#include "ipc_msg.h"
#include "logger.h"
#include <stdbool.h>
#include <stdint.h>
#include <zmq.h>

#define CAN_BUS_NOT_FOUND 0
#define CAN_BUS_DOWN      1
#define CAN_BUS_UP        2

static void *broadcaster = NULL;
static void *monitor = NULL;
static uint8_t clients = 0;

static ODR_t ipc_broadcast_data(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten);

static OD_extension_t ext = {
    .object = NULL,
    .read = OD_readOriginal,
    .write = ipc_broadcast_data,
};

int ipc_broadcast_init(void *context, OD_t *od) {
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

void ipc_broadcast_process(void) {
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

void ipc_broadcast_free(void) {
    if (broadcaster) {
        zmq_close(broadcaster);
        broadcaster = NULL;
    }
    if (monitor) {
        zmq_close(monitor);
        monitor = NULL;
    }
}

uint8_t ipc_clients_count(void) {
    return clients;
}

static ODR_t ipc_broadcast_data(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten) {
    ODR_t ac = OD_writeOriginal(stream, buf, count, countWritten);
    if ((ac == ODR_OK) && (stream->dataLength > 0) && (stream->dataLength <= IPC_STR_MAX_LEN)) {
        ipc_msg_od_t msg_od = {
            .header =
                {
                    .version = IPC_MSG_VERSION,
                    .id = IPC_MSG_ID_OD_WRITE,
                },
            .index = stream->index,
            .subindex = stream->subIndex,
            .buffer =
                {
                    .len = stream->dataLength,
                },
        };
        memcpy(&msg_od.buffer.data, buf, stream->dataLength);
        zmq_send(broadcaster, &msg_od, IPC_MSG_OD_MIN_LEN + stream->dataLength, 0);
        log_debug("od write index 0x%X subindex 0x%X", msg_od.index, msg_od.subindex);
    }
    return ac;
}

void ipc_broadcast_hb(uint8_t node_id, uint8_t state) {
    if ((clients == 0) || !broadcaster) {
        return;
    }
    ipc_msg_hb_recv_t msg_hb_recv = {
        .header =
            {
                .version = IPC_MSG_VERSION,
                .id = IPC_MSG_ID_HB_RECV,
            },
        .node_id = node_id,
        .state = state,
    };
    zmq_send(broadcaster, &msg_hb_recv, sizeof(ipc_msg_hb_recv_t), 0);
}

void ipc_broadcast_emcy(uint8_t node_id, uint16_t code, uint32_t info) {
    if ((clients == 0) || !broadcaster) {
        return;
    }
    ipc_msg_emcy_recv_t msg_emcy_recv = {
        .header =
            {
                .version = IPC_MSG_VERSION,
                .id = IPC_MSG_ID_EMCY_RECV,
            },
        .node_id = node_id,
        .code = code,
        .info = info,
    };
    zmq_send(broadcaster, &msg_emcy_recv, sizeof(ipc_msg_emcy_recv_t), 0);
}

static void ipc_broadcast_status(uint8_t state) {
    ipc_msg_bus_status_t msg_bus_status = {
        .header =
            {
                .version = IPC_MSG_VERSION,
                .id = IPC_MSG_ID_BUS_STATUS,
            },
        .state = state,
    };
    zmq_send(broadcaster, &msg_bus_status, sizeof(ipc_msg_bus_status_t), 0);
}

void ipc_broadcast_bus_status(CO_t *co) {
    if ((clients == 0) || !broadcaster) {
        return;
    }
    if (!co || !co->CANmodule || !co->CANmodule->CANinterfaces) {
        ipc_broadcast_status(CAN_BUS_NOT_FOUND);
        return;
    }

    static char file_path[64];
    sprintf(file_path, "/sys/class/net/%s/carrier", co->CANmodule->CANinterfaces[0].ifName);
    FILE *fp = fopen(file_path, "r");
    if (fp) {
        char up;
        fread(&up, 1, 1, fp);
        fclose(fp);
        if (up == '1') {
            ipc_broadcast_status(CAN_BUS_UP);
        } else {
            ipc_broadcast_status(CAN_BUS_DOWN);
        }
    } else {
        ipc_broadcast_status(CAN_BUS_NOT_FOUND);
    }
}
