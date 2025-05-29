#ifndef _IPC_RESPONDER_H_
#define _IPC_RESPONDER_H_

#include "CANopen.h"
#include "fcache.h"
#include <stdbool.h>
#include <stdint.h>

int ipc_respond_init(void *context);
void ipc_respond_process(CO_t *co, OD_t *od, CO_config_t *config, fcache_t *fread_cache);
void ipc_respond_free(void);

#endif
