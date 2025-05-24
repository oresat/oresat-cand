#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

void sleep_ms(uint32_t ms);
void sleep_us(uint64_t us); // usleep is obsolete and was removed in POSIX.1-2008

uint32_t get_uptime_s(void);
uint32_t get_uptime_ms(void);
uint64_t get_uptime_us(void);

uint32_t get_unix_time_s(void);

int run_bash_command(char *in, char *out, int out_max_len);

bool is_file(char *file_path);
bool is_file_in_dir(char *dir_path, char *file_name);
int copy_file(char *src, char *dest);
int move_file(char *src, char *dest);
int get_file_crc32(char *file_path, uint32_t *crc);
bool check_file_crc32_match(char *file_path_1, char *file_path_2);

bool is_dir(char *dir_path);
int mkdir_path(char *dir_path, mode_t mode);
int clear_dir(char *dir_path);
int files_in_dir(char *dir_path);

int path_join(char *head, char *tail, char *out, uint32_t out_len);

#endif
