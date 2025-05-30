#ifndef _IPC_MSG_H_
#define _IPC_MSG_H_

#include <stdint.h>

#define IPC_MSG_ID_LEN sizeof(uint8_t)

#define IPC_MSG_VERSION     0 // only increase on breaking changes
#define IPC_MSG_VERSION_LEN sizeof(uint8_t)

#define IPC_MSG_MAX_LEN 1000

#define ipc_str_len_t   uint8_t
#define IPC_STR_MAX_LEN ((1 << (sizeof(ipc_str_len_t) * 8)) - 1)

typedef enum {
    IPC_MSG_ID_EMCY_SEND = 0x0,
    IPC_MSG_ID_TPDO_SEND = 0x1,
    IPC_MSG_ID_OD_WRITE = 0x2,
    IPC_MSG_ID_SDO_READ = 0x3,
    IPC_MSG_ID_SDO_WRITE = 0x4,
    IPC_MSG_ID_ADD_FILE = 0x5,
    IPC_MSG_ID_HB_RECV = 0x6,
    IPC_MSG_ID_EMCY_RECV = 0x7,
    IPC_MSG_ID_SYNC_SEND = 0x8,
    IPC_MSG_ID_BUS_STATUS = 0x9,
    IPC_MSG_ID_SDO_READ_TO_FILE = 0xA,
    IPC_MSG_ID_SDO_WRITE_FROM_FILE = 0xB,
    IPC_MSG_ID_CONFIG = 0xC,
} ipc_msg_id_t;

typedef enum {
    IPC_MSG_ID_ERROR = 0x80,
    IPC_MSG_ID_ERROR_UNKNOWN_ID = 0x81,
    IPC_MSG_ID_ERROR_ABORT = 0x82,
} ipc_msg_id_error_t;

typedef struct __attribute__((packed)) {
    uint8_t version;
    uint8_t id;
} ipc_header_t;

typedef struct __attribute__((packed)) {
    ipc_str_len_t len;
    uint8_t data[IPC_STR_MAX_LEN];
} ipc_bytes_t;

typedef struct __attribute__((packed)) {
    ipc_str_len_t len;
    char data[IPC_STR_MAX_LEN];
} ipc_str_t;

typedef struct __attribute__((packed)) {
    ipc_header_t header;
    uint16_t code;
    uint32_t info;
} ipc_msg_emcy_send_t;

typedef struct __attribute__((packed)) {
    ipc_header_t header;
    uint8_t number;
} ipc_msg_tpdo_send_t;

typedef struct __attribute__((packed)) {
    ipc_header_t header;
    uint16_t index;
    uint8_t subindex;
    ipc_bytes_t buffer;
} ipc_msg_od_t;
#define IPC_MSG_OD_MIN_LEN (offsetof(ipc_msg_od_t, buffer) + sizeof(ipc_str_len_t))

typedef struct __attribute__((packed)) {
    ipc_header_t header;
    uint8_t node_id;
    uint16_t index;
    uint8_t subindex;
    ipc_bytes_t buffer;
} ipc_msg_sdo_t;
#define IPC_MSG_SDO_MIN_LEN (offsetof(ipc_msg_sdo_t, buffer) + sizeof(ipc_str_len_t))

typedef struct __attribute__((packed)) {
    ipc_header_t header;
    ipc_str_t path;
} ipc_msg_file_t;
#define IPC_MSG_FILE_MIN_LEN (offsetof(ipc_msg_file_t, path) + sizeof(ipc_str_len_t))

typedef struct __attribute__((packed)) {
    ipc_header_t header;
    uint8_t state;
} ipc_msg_bus_status_t;

typedef struct __attribute__((packed)) {
    ipc_header_t header;
    uint8_t node_id;
    uint16_t index;
    uint8_t subindex;
    ipc_str_t path;
} ipc_msg_sdo_file_t;
#define IPC_MSG_SDO_FILE_MIN_LEN (offsetof(ipc_msg_sdo_file_t, path) + sizeof(ipc_str_len_t))

typedef struct __attribute__((packed)) {
    ipc_header_t header;
    uint8_t node_id;
    uint16_t code;
    uint32_t info;
} ipc_msg_emcy_recv_t;

typedef struct __attribute__((packed)) {
    ipc_header_t header;
    uint8_t node_id;
    uint8_t state;
} ipc_msg_hb_recv_t;

typedef struct __attribute__((packed)) {
    ipc_header_t header;
    uint8_t error;
} ipc_msg_error_t;

typedef struct __attribute__((packed)) {
    ipc_header_t header;
    uint8_t id;
} ipc_msg_error_id_t;

typedef struct __attribute__((packed)) {
    ipc_header_t header;
    uint32_t code;
} ipc_msg_error_abort_t;

#endif
