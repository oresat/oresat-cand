#ifndef _IPC_H_
#define _IPC_H_

#include "CANopen.h"

#define IPC_PORT "5555"

#define MSG_ID_SEND_EMCY        0x00
#define MSG_ID_SEND_TPDO        0x01
#define MSG_ID_OD_READ          0x02
#define MSG_ID_OD_WRITE         0x03
#define MSG_ID_SDO_READ         0x04
#define MSG_ID_SDO_WRITE        0x05
#define MSG_ID_ERROR_UNKNOWN_ID 0x80
#define MSG_ID_ERROR_LENGTH     0x81
#define MSG_ID_ERROR_TPDO_NUM   0x82
#define MSG_ID_ERROR_OD_ABORT   0x83
#define MSG_ID_ERROR_SDO_ABORT  0x84

typedef struct {
    OD_t *od;
    CO_t *co;
    CO_config_t *config;
} ipc_args_t;

typedef struct __attribute((packed)) {
    uint8_t id;
    uint16_t index;
    uint8_t subindex;
    uint8_t dtype;
} ipc_msg_od_t;

typedef struct __attribute((packed)) {
    uint8_t id;
    uint8_t node_id;
    uint16_t index;
    uint8_t subindex;
    uint8_t dtype;
} ipc_msg_sdo_t;

void ipc_init(ipc_args_t *ipc_args);
void ipc_free(void);

#endif
