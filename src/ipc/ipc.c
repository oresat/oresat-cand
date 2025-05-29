#include "ipc.h"
#include "CANopen.h"
#include "ipc_broadcast.h"
#include "ipc_consume.h"
#include "ipc_respond.h"
#include <zmq.h>

static void *context = NULL;

void ipc_init(OD_t *od) {
    context = zmq_ctx_new();
    if (context) {
        ipc_broadcast_init(context, od);
        ipc_consume_init(context);
        ipc_respond_init(context);
    }
}

void ipc_free(void) {
    if (context) {
        ipc_broadcast_free();
        ipc_consume_free();
        ipc_respond_free();
        zmq_ctx_term(context);
    }
}
