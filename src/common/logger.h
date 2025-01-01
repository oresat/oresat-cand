#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <stdint.h>
#include <sys/syslog.h>

int log_level_get(void);
void log_level_set(int level);
char *log_level_get_str(int level);
void log_message(int level, char *file, uint32_t line, const char *fmt, ...);

#define log_debug(fmt, ...) log_message(LOG_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define log_info(fmt, ...) log_message(LOG_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define log_notice(fmt, ...) log_message(LOG_NOTICE, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define log_warning(fmt, ...) log_message(LOG_WARNING, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define log_error(fmt, ...) log_message(LOG_ERR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define log_critical(fmt, ...) log_message(LOG_CRIT, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define log_alert(fmt, ...) log_message(LOG_ALERT, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define log_emergency(fmt, ...) log_message(LOG_EMERG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#endif
