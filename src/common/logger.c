#include <libgen.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/syslog.h>
#include "logger.h"

static char *LOG_LEVEL_STR[] = {
    "DEBUG",
    "INFO",
    "NOTICE",
    "WARNINNG",
    "ERROR",
    "CRITIAL",
    "ALERT",
    "EMERGENCY",
    "UNKNOWN",
};

static int log_level = LOG_INFO;

int log_level_get(void) {
    return log_level;
}

void log_level_set(int level) {
    log_level = level;
}

char *log_level_get_str(int level) {
    char *name = NULL;
    switch (level) {
        case LOG_DEBUG:
            name = LOG_LEVEL_STR[0];
            break;
        case LOG_INFO:
            name = LOG_LEVEL_STR[1];
            break;
        case LOG_NOTICE:
            name = LOG_LEVEL_STR[2];
            break;
        case LOG_WARNING:
            name = LOG_LEVEL_STR[3];
            break;
        case LOG_ERR:
            name = LOG_LEVEL_STR[4];
            break;
        case LOG_CRIT:
            name = LOG_LEVEL_STR[5];
            break;
        case LOG_ALERT:
            name = LOG_LEVEL_STR[6];
            break;
        case LOG_EMERG:
            name = LOG_LEVEL_STR[7];
            break;
        default:
            name = LOG_LEVEL_STR[8];
            break;
    }
    return name;
}

void log_message(int level, char *file, uint32_t line, const char *fmt, ...) {
    if (level <= log_level) {
        return;
    }

    va_list args1;
    va_start(args1, fmt);
    va_list args2;
    va_copy(args2, args1);
    char buf[1 + vsnprintf(NULL, 0, fmt, args1)];
    va_end(args1);
    vsnprintf(buf, sizeof buf, fmt, args2);
    va_end(args2);

    printf("%s: %s:%d - %s\n", log_level_get_str(level), basename(file), line, buf);
}
