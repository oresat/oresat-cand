#ifndef _OS_COMMAND_EXT_H_
#define _OS_COMMAND_EXT_H_

#include "301/CO_ODinterface.h"

#define OS_CMD_INDEX 0x1023
#define OS_CMD_SUBINDEX_COMMAND 0x1
#define OS_CMD_SUBINDEX_STATUS 0x2
#define OS_CMD_SUBINDEX_REPLY 0x3

/** OS command status values defined by CiA 301 */
typedef enum {
    OS_CMD_NO_ERROR_NO_REPLY = 0x00,
    OS_CMD_NO_ERROR_REPLY = 0x01,
    OS_CMD_ERROR_NO_REPLY = 0x02,
    OS_CMD_ERROR_REPLY = 0x03,
    OS_CMD_EXECUTING = 0xFF,
} OS_CMD_ENUM;

void os_command_extension_init(OD_t *od);
void os_command_extension_free(void);

#endif
