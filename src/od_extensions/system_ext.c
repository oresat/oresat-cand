#include "system_ext.h"
#include "301/CO_ODinterface.h"
#include "OD.h"
#include "logger.h"
#include "system.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/param.h>
#include <sys/statvfs.h>
#include <sys/sysinfo.h>
#include <unistd.h>

static ODR_t system_read(OD_stream_t *stream, void *buf, OD_size_t count, OD_size_t *countRead);
static ODR_t system_write(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten);

static OD_extension_t ext = {
    .object = NULL,
    .read = system_read,
    .write = system_write,
};

void system_extension_init(OD_t *od) {
    OD_entry_t *entry = OD_find(od, OD_INDEX_SYSTEM);
    if (entry != NULL) {
        OD_extension_init(entry, &ext);
    } else {
        log_critical("could not find os command enty 0x(%X)", OD_INDEX_SYSTEM);
    }
}

static ODR_t system_read(OD_stream_t *stream, void *buf, OD_size_t count, OD_size_t *countRead) {
    ODR_t r = ODR_OK;
    switch (stream->subIndex) {
    case 0: {
        r = OD_readOriginal(stream, buf, count, countRead);
        break;
    }
    case OD_SUBINDEX_SYSTEM_RESET: {
        r = ODR_WRITEONLY;
        break;
    }
    case OD_SUBINDEX_SYSTEM_STORAGE_PERCENT: {
        struct statvfs fiData;
        uint8_t usage = 0;
        if ((statvfs("/", &fiData)) >= 0) {
            usage = (uint8_t)(((fiData.f_blocks - fiData.f_bavail) * 100) / fiData.f_blocks);
        }
        memcpy(buf, &usage, 1);
        *countRead = sizeof(usage);
        break;
    }
    case OD_SUBINDEX_SYSTEM_RAM_PERCENT: {
        struct sysinfo info;
        uint8_t usage = 0;
        if ((sysinfo(&info)) >= 0) {
            usage = (uint8_t)(((info.totalram - info.freeram) * 100) / info.totalram);
        }
        memcpy(buf, &usage, 1);
        *countRead = sizeof(usage);
        break;
    }
    case OD_SUBINDEX_SYSTEM_UNIX_TIME: {
        uint32_t unix_time = get_unix_time_s();
        memcpy(buf, &unix_time, 1);
        *countRead = sizeof(unix_time);
        break;
    }
    case OD_SUBINDEX_SYSTEM_UPTIME: {
        uint32_t uptime = get_uptime_s();
        memcpy(buf, &uptime, 1);
        *countRead = sizeof(uptime);
        break;
    }
    case OD_SUBINDEX_SYSTEM_POWER_CYCLES: {
        uint16_t powercycles = 0;
        memcpy(buf, &powercycles, 1);
        *countRead = sizeof(powercycles);
        break;
    }
    case OD_SUBINDEX_SYSTEM_BOOT_SELECT: {
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
    case OD_SUBINDEX_SYSTEM_STORAGE_PERCENT:
    case OD_SUBINDEX_SYSTEM_RAM_PERCENT:
    case OD_SUBINDEX_SYSTEM_UNIX_TIME:
    case OD_SUBINDEX_SYSTEM_UPTIME:
    case OD_SUBINDEX_SYSTEM_POWER_CYCLES:
    case OD_SUBINDEX_SYSTEM_BOOT_SELECT:
        r = ODR_READONLY;
        break;
    case OD_SUBINDEX_SYSTEM_RESET:
        r = OD_writeOriginal(stream, buf, count, countWritten);
        break;
    default:
        r = ODR_SUB_NOT_EXIST;
        break;
    }
    return r;
}
