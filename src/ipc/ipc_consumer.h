#ifndef _IPC_CONSUMER_H_
#define _IPC_CONSUMER_H_

#include "CANopen.h"
#include <stdbool.h>
#include <stdint.h>

int ipc_conumer_init(void *context);
void ipc_consumer_process(CO_t *co, OD_t *od, CO_config_t *base_config, CO_config_t *config, bool *reset);
void ipc_conumer_free(void);

#endif
