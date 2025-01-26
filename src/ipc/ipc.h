#ifndef _IPC_H_
#define _IPC_H_

#include "CANopen.h"

typedef struct {
    OD_t *od;
    CO_t *co;
    CO_config_t *config;
} ipc_args_t;

void ipc_init(ipc_args_t *ipc_args);
void ipc_free(void);

#endif
