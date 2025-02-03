#ifndef _IPC_CAN_EVENT_H_
#define _IPC_CAN_EVENT_H_

#include "CANopen.h"

typedef struct {
    uint8_t *buffer;
    size_t buffer_len;
    uint8_t dtype;
    float low_limit;
    float high_limit;
} ipc_can_event_t;

void ipc_can_event_init(void *context, CO_t *co);
void ipc_can_event_free(void);

ODR_t ipc_can_event_write_cb(OD_stream_t* stream, const void* buf, OD_size_t count, OD_size_t* countWritten);

#endif
