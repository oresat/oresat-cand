#ifndef _IPC_CONSUMER_H_
#define _IPC_CONSUMER_H_

#include "CANopen.h"
#include <stdbool.h>
#include <stdint.h>

int ipc_consume_init(void *context);
void ipc_consume_process(CO_t *co, OD_t *od, CO_config_t *base_config, CO_config_t *config, char *od_config_path,
                         bool *reset);
void ipc_consume_free(void);

#endif
