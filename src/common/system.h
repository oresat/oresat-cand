#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

void sleep_ms(uint32_t ms);
void sleep_us(uint64_t us); // usleep is obsolete and was removed in POSIX.1-2008

uint32_t get_uptime(void);
uint32_t get_uptime_ms(void);
uint64_t get_uptime_us(void);

uint32_t get_unix_time_s(void);

int run_bash_command(char *in, char *out, int out_max_len);

bool is_file(const char *file_path);
bool is_file_in_dir(const char *dir_path, const char *file_name);
int copy_file(const char *src, const char *dest);
int move_file(const char *src, const char *dest);

bool is_dir(const char *dir_path);
int mkdir_path(const char *dir_path, mode_t mode);
int clear_dir(const char *dir_path);
int files_in_dir(const char *dir_path);

int path_join(const char *head, const char *tail, char *out, uint32_t out_len);

#endif
