#include "ipc.h"
#include "CANopen.h"
#include "ipc_broadcaster.h"
#include "ipc_consumer.h"
#include "ipc_responder.h"
#include <zmq.h>

static void *context = NULL;

void ipc_init(OD_t *od) {
    context = zmq_ctx_new();
    if (context) {
        ipc_broadcaster_init(context, od);
        ipc_conumer_init(context);
        ipc_responder_init(context);
    }
}

void ipc_free(void) {
    if (context) {
        ipc_broadcaster_free();
        ipc_conumer_free();
        ipc_responder_free();
        zmq_ctx_term(context);
    }
}
