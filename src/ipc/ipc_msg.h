#ifndef _IPC_MSG_H_
#define _IPC_MSG_H_

#include <stdint.h>

#define IPC_MSG_VERSION 0

#define IPC_BUFFER_SIZE 1000

#define IPC_STR_MAX_LEN 255

typedef enum {
    IPC_MSG_ID_EMCY_SEND = 0x00,
    IPC_MSG_ID_TPDO_SEND = 0x01,
    IPC_MSG_ID_OD_WRITE = 0x02,
    IPC_MSG_ID_SDO_READ = 0x03,
    IPC_MSG_ID_SDO_WRITE = 0x04,
    IPC_MSG_ID_ADD_FILE = 0x05,
    IPC_MSG_ID_HB_RECV = 0x06,
    IPC_MSG_ID_EMCY_RECV = 0x07,
    IPC_MSG_ID_SYNC_SEND = 0x08,
    IPC_MSG_ID_BUS_STATUS = 0x09,
    IPC_MSG_ID_SDO_READ_FILE = 0x0A,
    IPC_MSG_ID_SDO_WRITE_FILE = 0x0B,
    IPC_MSG_ID_SDO_LIST_FILES = 0x0C,
    IPC_MSG_ID_CONFIG = 0x0D,
} ipc_msg_id_t;

typedef enum {
    IPC_MSG_ID_ERROR = 0x80,
    IPC_MSG_ID_ERROR_UNKNOWN_ID = 0x81,
    IPC_MSG_ID_ERROR_ABORT = 0x82,
} ipc_msg_id_error_t;

typedef struct __attribute__((packed)) {
    uint16_t code;
    uint32_t info;
} ipc_msg_emcy_send_t;

typedef struct __attribute__((packed)) {
    uint16_t index;
    uint8_t subindex;
    uint8_t buffer_len;
    uint8_t buffer[IPC_STR_MAX_LEN];
} ipc_msg_od_t;

typedef struct __attribute__((packed)) {
    uint8_t node_id;
    uint16_t index;
    uint8_t subindex;
    uint8_t buffer_len;
    uint8_t buffer[IPC_STR_MAX_LEN];
} ipc_msg_sdo_t;

typedef struct __attribute__((packed)) {
    uint8_t file_path_len;
    char file_path[IPC_STR_MAX_LEN];
} ipc_msg_add_file_t;

typedef struct __attribute__((packed)) {
    uint8_t node_id;
    uint16_t state;
} ipc_msg_hb_recv_t;

typedef struct __attribute__((packed)) {
    uint8_t node_id;
    uint16_t state;
    uint16_t code;
    uint32_t info;
} ipc_msg_emcy_recv_t;

typedef struct {
    uint8_t node_id;
    uint8_t remote_file_path_len;
    char remote_file_path[IPC_STR_MAX_LEN];
    uint8_t local_file_path_len;
    char local_file_path[IPC_STR_MAX_LEN];
} ipc_msg_sdo_readwrite_file_t;

typedef struct __attribute__((packed)) {
    uint8_t node_id;
    uint8_t files_len;
    char files[IPC_STR_MAX_LEN];
} ipc_msg_sdo_list_files_t;

typedef struct __attribute__((packed)) {
    uint8_t file_len;
    char file[IPC_STR_MAX_LEN];
} ipc_msg_config_t;

#endif
