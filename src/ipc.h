#ifndef _IPC_H_
#define _IPC_H_

#include <stdint.h>
#include "CANopen.h"

#define IPC_MSG_ID_EMCY_SEND            0x00
#define IPC_MSG_ID_TPDO_SEND            0x01
#define IPC_MSG_ID_OD_READ              0x02
#define IPC_MSG_ID_OD_WRITE             0x03
#define IPC_MSG_ID_SDO_READ             0x04
#define IPC_MSG_ID_SDO_WRITE            0x05
#define IPC_MSG_ID_REQUEST_PORT         0x06
#define IPC_MSG_ID_REQUEST_OWNERSHIP    0x07

#define IPC_MSG_ID_ERROR                0x80
#define IPC_MSG_ID_ERROR_UNKNOWN_ID     0x81
#define IPC_MSG_ID_ERROR_ABORT          0x82

typedef struct __attribute((packed)) {
    uint8_t id;
    uint16_t code;
    uint32_t info;
} ipc_msg_emcy_t;

typedef struct __attribute((packed)) {
    uint8_t id;
    uint8_t num;
} ipc_msg_tpdo_t;

typedef struct __attribute((packed)) {
    uint8_t id;
    uint16_t index;
    uint8_t subindex;
} ipc_msg_od_t;

typedef struct __attribute((packed)) {
    uint8_t id;
    uint8_t node_id;
    uint16_t index;
    uint8_t subindex;
} ipc_msg_sdo_t;

typedef struct __attribute((packed)) {
    uint8_t id;
    uint32_t port;
} ipc_msg_request_port_t;

typedef struct __attribute((packed)) {
    uint8_t id;
    uint16_t index;
    uint8_t subindex;
    bool read;
    bool write;
} ipc_msg_request_ownership_t;

typedef struct __attribute((packed)) {
    uint8_t id;
    uint32_t abort_code;
} ipc_msg_error_abort_t;

void ipc_init(OD_t *od);
void ipc_responder_process(CO_t* co, OD_t* od, CO_config_t *config);
void ipc_consumer_process(CO_t* co, OD_t* od, CO_config_t *config);
void ipc_free(OD_t *od);

#endif
