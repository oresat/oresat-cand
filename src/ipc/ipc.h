#ifndef _IPC_H_
#define _IPC_H_

#include "CANopen.h"

#define IPC_PORT "5555"

#define IPC_MSG_ID_EMCY_SEND        0x00
#define IPC_MSG_ID_TPDO_SEND        0x01
#define IPC_MSG_ID_OD_READ          0x02
#define IPC_MSG_ID_OD_WRITE         0x03
#define IPC_MSG_ID_SDO_READ         0x04
#define IPC_MSG_ID_SDO_WRITE        0x05
#define IPC_MSG_ID_EMCY_RECV        0x06
#define IPC_MSG_ID_ERROR_UNKNOWN_ID 0x80
#define IPC_MSG_ID_ERROR_LENGTH     0x81
#define IPC_MSG_ID_ERROR_TPDO_NUM   0x82
#define IPC_MSG_ID_ERROR_OD_ABORT   0x83
#define IPC_MSG_ID_ERROR_SDO_ABORT  0x84

typedef struct __attribute((packed)) {
    uint8_t id;
    uint16_t code;
    uint32_t info;
} ipc_msg_emcy_t;

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

typedef struct __attribute((packed)) {
    uint8_t id;
    uint8_t node_id;
    uint16_t error_code;
    uint8_t error_register;
    uint8_t error_bit;
    uint32_t info_code;
} ipc_msg_emcy_recv_t;

void ipc_init(CO_t *co);
void ipc_process(CO_t* co, OD_t* od, CO_config_t *config);
void ipc_free(void);

#endif
