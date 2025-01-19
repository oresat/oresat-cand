#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>
#include "301/CO_ODinterface.h"
#include "logger.h"
#include "system.h"
#include "od_ext.h"
#include "os_command_ext.h"

#define COMMAND_BUFFER_LEN 4096
#define REPLY_BUFFER_LEN 10024

static bool running;
static pthread_t thread_id;
static uint8_t status = OS_CMD_ERROR_NO_REPLY;
static char command[COMMAND_BUFFER_LEN] = { 0 };
static char reply[REPLY_BUFFER_LEN] = { 0 };

static ODR_t os_command_write(OD_stream_t* stream, const void* buf, OD_size_t count, OD_size_t* countWritten);
static ODR_t os_command_read(OD_stream_t* stream, void* buf, OD_size_t count, OD_size_t* countRead);
static void* os_command_thread(void* arg);

static OD_extension_t ext = {
    .object = NULL,
    .read = os_command_read,
    .write = os_command_write,
};

void os_command_extension_init(OD_t *od) {
    OD_entry_t *entry = OD_find(od, OS_CMD_INDEX);
    if (entry != NULL) {
        OD_extension_init(entry, &ext);
        pthread_create(&thread_id, NULL, os_command_thread, NULL);
    } else {
        log_critical("could not find os command enty 0x(%X)", OS_CMD_INDEX);
    }
}

void os_command_extension_free(void) {
    running = false;
    pthread_join(thread_id, NULL);
}

static void* os_command_thread(void* arg) {
    (void)arg;
    running = true;
    unsigned int log_size = 50;
    char message[log_size];
    message[log_size - 4] = '.';
    message[log_size - 3] = '.';
    message[log_size - 2] = '.';
    message[log_size - 1] = '\0';

    while (running) {
        sleep_ms(250);
        if (status != OS_CMD_EXECUTING) {
            continue; // nothing todo
        }

        if (strlen(command) == 0) {
            status = OS_CMD_ERROR_NO_REPLY;
        } else {
            strncpy(message, command, MIN(strlen(command) + 1, log_size - 4));
            log_info("running os command: %s", message);
            int reply_len = run_bash_command(command, reply, REPLY_BUFFER_LEN);
            if (reply_len < 0) {
                reply[0] = '\0';
                status = OS_CMD_ERROR_NO_REPLY;
            } else if (reply_len == 0) {
                reply[0] = '\0';
                status = OS_CMD_NO_ERROR_NO_REPLY;
            } else {
                status = OS_CMD_NO_ERROR_REPLY;
                reply[reply_len - 1] = '\0';
            }
        }
    }
    return NULL;
}

static ODR_t os_command_read(OD_stream_t* stream, void* buf, OD_size_t count, OD_size_t* countRead) {
    ODR_t r = ODR_OK;
    if (stream->subIndex == 0) {
        r = OD_readOriginal(stream, buf, count, countRead);
    } else if (stream->subIndex == OS_CMD_SUBINDEX_COMMAND) {
        r = od_ext_read_data(stream, buf, count, countRead, command, strlen(command) + 1);
    } else if (stream->subIndex == OS_CMD_SUBINDEX_STATUS) {
        memcpy(buf, &status, 1);
        *countRead = 1;
    } else if (stream->subIndex == OS_CMD_SUBINDEX_REPLY) {
        if (reply[0] == '\0') {
            r = ODR_NO_DATA;
        } else {
            r = od_ext_read_data(stream, buf, count, countRead, reply, strlen(reply) + 1);
        }
    }
    return r;
}

static ODR_t os_command_write(OD_stream_t* stream, const void* buf, OD_size_t count, OD_size_t* countWritten) {
    ODR_t r = ODR_READONLY;
    if (stream->subIndex == OS_CMD_SUBINDEX_COMMAND) {
        if (status == OS_CMD_EXECUTING) {
            return ODR_DATA_LOC_CTRL;
        }
        size_t command_len = 0;
        r = od_ext_write_data(stream, buf, count, countWritten, command, COMMAND_BUFFER_LEN - 1, &command_len);
        if (r == ODR_OK) {
            command[command_len] = '\0';
            command_len++;
            status = OS_CMD_EXECUTING;
        }
    }
    return r;
}
