#ifndef _FCACHE_H_
#define _FCACHE_H_

#include <linux/limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    char dir_path[PATH_MAX];
    pthread_mutex_t mutex;
} fcache_t;

fcache_t* fcache_init(char *dir_path);
void fcache_free(fcache_t *cache);

int fcache_add(fcache_t *cache, char *file_path, bool consume);
int fcache_add_new(fcache_t *cache, char *file_name, uint8_t *file_data, uint32_t file_data_len);
int fcache_delete(fcache_t *cache, char *file_name);
int fcache_copy(fcache_t *cache, char *file_name, char *dest_dir);
int fcache_move(fcache_t *cache, char *file_name, char *dest_dir);

uint32_t fcache_size(fcache_t *cache);

bool fcache_file_exist(fcache_t *cache, char *file_name);

char* fcache_list_files_as_json(fcache_t *cache); // non-NULL results must be freed

#endif
