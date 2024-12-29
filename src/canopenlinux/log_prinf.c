#include <libgen.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/syslog.h>
#include <sys/time.h>
#include <libgen.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

char *LOG_LEVEL_STR[] = {
    "EMERGENCY",
    "ALERT",
    "CRITIAL",
    "ERROR",
    "WARNINNG",
    "NOTICE",
    "INFO",
    "DEBUG",
};

void log_printf(int priority, const char* format, ...) {
    char *level = NULL;
    switch (priority) {
        case LOG_EMERG:
            level = LOG_LEVEL_STR[0];
            break;
        case LOG_ALERT:
            level = LOG_LEVEL_STR[1];
            break;
        case LOG_CRIT:
            level = LOG_LEVEL_STR[2];
            break;
        case LOG_ERR:
            level = LOG_LEVEL_STR[3];
            break;
        case LOG_WARNING:
            level = LOG_LEVEL_STR[4];
            break;
        case LOG_NOTICE:
            level = LOG_LEVEL_STR[5];
            break;
        case LOG_INFO:
            level = LOG_LEVEL_STR[6];
            break;
        case LOG_DEBUG:
            level = LOG_LEVEL_STR[7];
            break;
    }

    va_list args1;
    va_start(args1, format);
    va_list args2;
    va_copy(args2, args1);
    char buf[1+vsnprintf(NULL, 0, format, args1)];
    va_end(args1);
    vsnprintf(buf, sizeof(buf), format, args2);
    va_end(args2);

    printf("%s: %s\n", level, buf);
}
