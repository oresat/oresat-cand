#ifndef _IPC_BROADCASTER_H_
#define _IPC_BROADCASTER_H_

#include "CANopen.h"
#include <stdbool.h>
#include <stdint.h>

int ipc_broadcaster_init(void *context, OD_t *od);
void ipc_broadcaster_process(void);
void ipc_broadcaster_free(void);

void ipc_broadcaster_hb(uint8_t node_id, uint8_t state);
void ipc_broadcaster_emcy(uint8_t node_id, uint16_t code, uint32_t info);
void ipc_broadcaster_bus_status(CO_t *co);

uint8_t ipc_clients_count(void);

#endif
