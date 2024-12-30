#ifndef _LOG_PRINTF_H_
#define _LOG_PRINTF_H_

void log_printf(int priority, const char* format, ...);
void log_printf_set_level(int priority);

#endif
