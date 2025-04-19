#include "CO_error.h"
#include "logger.h"
#include <stdarg.h>
#include <stdio.h>
#include <sys/syslog.h>

/*
 * This function is defined in CANopenLinux/CO_error.h,
 * so it can't be a #define macro
 *
 * This will format the message close to the style defined in logger.c
 */
void log_printf(int priority, const char *format, ...) {
    if (priority > log_level_get()) {
        return;
    }

    va_list args1;
    va_start(args1, format);
    va_list args2;
    va_copy(args2, args1);
    char buf[1 + vsnprintf(NULL, 0, format, args1)];
    va_end(args1);
    vsnprintf(buf, sizeof(buf), format, args2);
    va_end(args2);

    printf("%s: %s\n", log_level_get_str(priority), buf);
}
