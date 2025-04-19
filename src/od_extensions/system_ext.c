#include "system_ext.h"
#include "301/CO_ODinterface.h"
#include "logger.h"
#include "system.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/param.h>
#include <sys/statvfs.h>
#include <sys/sysinfo.h>
#include <unistd.h>

#define SYS_INDEX 0x3003

#define SYS_SUBINDEX_RESET           0x1
#define SYS_SUBINDEX_STORAGE_PERCENT 0x2
#define SYS_SUBINDEX_RAM_PERCENT     0x3
#define SYS_SUBINDEX_UNIX_TIME       0x4
#define SYS_SUBINDEX_UPTIME          0x5
#define SYS_SUBINDEX_POWER_CYCLES    0x6
#define SYS_SUBINDEX_BOOT_SELECT     0x9

static ODR_t system_read(OD_stream_t *stream, void *buf, OD_size_t count, OD_size_t *countRead);
static ODR_t system_write(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten);

static OD_extension_t ext = {
    .object = NULL,
    .read = system_read,
    .write = system_write,
};

void system_extension_init(OD_t *od) {
    OD_entry_t *entry = OD_find(od, SYS_INDEX);
    if (entry != NULL) {
        OD_extension_init(entry, &ext);
    } else {
        log_critical("could not find os command enty 0x(%X)", SYS_INDEX);
    }
}

static ODR_t system_read(OD_stream_t *stream, void *buf, OD_size_t count, OD_size_t *countRead) {
    ODR_t r = ODR_OK;
    switch (stream->subIndex) {
    case 0: {
        r = OD_readOriginal(stream, buf, count, countRead);
        break;
    }
    case SYS_SUBINDEX_RESET: {
        r = ODR_WRITEONLY;
        break;
    }
    case SYS_SUBINDEX_STORAGE_PERCENT: {
        struct statvfs fiData;
        uint8_t usage = 0;
        if ((statvfs("/", &fiData)) >= 0) {
            usage = (uint8_t)(((fiData.f_blocks - fiData.f_bavail) * 100) / fiData.f_blocks);
        }
        memcpy(buf, &usage, 1);
        *countRead = sizeof(usage);
        break;
    }
    case SYS_SUBINDEX_RAM_PERCENT: {
        struct sysinfo info;
        uint8_t usage = 0;
        if ((sysinfo(&info)) >= 0) {
            usage = (uint8_t)(((info.totalram - info.freeram) * 100) / info.totalram);
        }
        memcpy(buf, &usage, 1);
        *countRead = sizeof(usage);
        break;
    }
    case SYS_SUBINDEX_UNIX_TIME: {
        uint32_t unix_time = get_unix_time_s();
        memcpy(buf, &unix_time, 1);
        *countRead = sizeof(unix_time);
        break;
    }
    case SYS_SUBINDEX_UPTIME: {
        uint32_t uptime = get_uptime_s();
        memcpy(buf, &uptime, 1);
        *countRead = sizeof(uptime);
        break;
    }
    case SYS_SUBINDEX_POWER_CYCLES: {
        uint16_t powercycles = 0;
        memcpy(buf, &powercycles, 1);
        *countRead = sizeof(powercycles);
        break;
    }
    case SYS_SUBINDEX_BOOT_SELECT: {
        uint8_t boot_select = 0;
        memcpy(buf, &boot_select, 1);
        *countRead = sizeof(boot_select);
        break;
    }
    default:
        r = ODR_SUB_NOT_EXIST;
        break;
    }

    return r;
}

static ODR_t system_write(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten) {
    ODR_t r = ODR_OK;
    switch (stream->subIndex) {
    case 0:
    case SYS_SUBINDEX_STORAGE_PERCENT:
    case SYS_SUBINDEX_RAM_PERCENT:
    case SYS_SUBINDEX_UNIX_TIME:
    case SYS_SUBINDEX_UPTIME:
    case SYS_SUBINDEX_POWER_CYCLES:
    case SYS_SUBINDEX_BOOT_SELECT:
        r = ODR_READONLY;
        break;
    case SYS_SUBINDEX_RESET:
        r = OD_writeOriginal(stream, buf, count, countWritten);
        break;
    default:
        r = ODR_SUB_NOT_EXIST;
        break;
    }
    return r;
}
