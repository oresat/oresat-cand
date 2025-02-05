#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zmq.h>
#include "CANopen.h"
#include "CO_ODinterface.h"
#include "logger.h"
#include "ipc.h"
#include "ipc_can_event.h"

#define IPC_CAN_EVENT_PORT "5556"
#define IS_STR_DTYPE(dtype) (dtype == 0x9 || dtype == 0xA || dtype == 0xF)

static void ipc_can_event_od_change(uint8_t index, uint8_t subindex, uint8_t dtype, uint8_t *data, size_t len);
static void ipc_can_event_emcy_cb(const uint16_t ident, const uint16_t error_code, const uint8_t error_register, const uint8_t error_bit, const uint32_t info_code);

static void *publisher = NULL;

void ipc_can_event_init(void *context, CO_t *co) {
    if (!context || !co) {
        log_error("ipc_can_event_init arg invalid");
        return;
    }

    publisher = zmq_socket(context, ZMQ_PUB);
    zmq_bind(publisher, "tcp://*:"IPC_CAN_EVENT_PORT);

    if (co->em) {
        CO_EM_initCallbackRx(co->em, ipc_can_event_emcy_cb);
    }
}

void ipc_can_event_free(void) {
    zmq_close(publisher);
}

static void ipc_can_event_od_change(uint8_t index, uint8_t subindex, uint8_t dtype, uint8_t *data, size_t len) {
    uint8_t buffer[len + sizeof(ipc_msg_od_t)];
    ipc_msg_od_t msg_od = {
        .id = IPC_MSG_ID_OD_WRITE,
        .index = index,
        .subindex = subindex,
        .dtype = dtype,
    };
    memcpy(buffer, &msg_od, sizeof(ipc_msg_od_t));
    memcpy(buffer, &data[sizeof(ipc_msg_od_t)], len);
    log_debug("od change index 0x%X subindex 0x%X", index, subindex);
    zmq_send(publisher, &data, sizeof(ipc_msg_od_t) + len, 0);
}

static void ipc_can_event_emcy_cb(const uint16_t ident, const uint16_t error_code, const uint8_t error_register, const uint8_t error_bit, const uint32_t info_code) {
    ipc_msg_emcy_recv_t msg_emcy_recv = {
        .id = IPC_MSG_ID_EMCY_RECV,
        .node_id = ident & 0x7F,
        .error_code = error_code,
        .error_register = error_register,
        .error_bit = error_bit,
        .info_code = info_code,
    };
    log_debug("emcy recv node 0x%X code 0x%X info 0x%X", ident & 0x7F, error_code, info_code);
    zmq_send(publisher, &msg_emcy_recv, sizeof(msg_emcy_recv), 0);
}

ODR_t ipc_can_event_write_cb(OD_stream_t* stream, const void* buf, OD_size_t count, OD_size_t* countWritten) {
    ODR_t r = OD_writeOriginal(stream, buf, count, countWritten);
    if (r == ODR_OK) {
        ipc_can_event_t *ipc_can_event = (ipc_can_event_t *)stream->object;
        ipc_can_event_od_change(stream->index, stream->subIndex, ipc_can_event->dtype, stream->dataOrig, stream->dataLength);
    }

    return r;
}
