#include <dirent.h>
#include <errno.h>
#include <linux/limits.h>
#include <libgen.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include "system.h"
#include "fcache.h"

fcache_t* fcache_init(char *dir_path)
{
    int r = mkdir_path(dir_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (r < 0) {
        return NULL;
    }

    fcache_t *cache = (fcache_t *)malloc(sizeof(fcache_t));;
    if (cache) {
        pthread_mutex_init(&cache->mutex, NULL);
        strncpy(cache->dir_path, dir_path, strlen(dir_path) + 1);
    }
    return cache;
}

void fcache_free(fcache_t *cache) {
    if (!cache) {
        return;
    }
    pthread_mutex_destroy(&cache->mutex);
    free(cache);
}

int fcache_add(fcache_t *cache, char *file_path, bool consume)
{
    if (!cache || !is_file(file_path)) {
        return -EINVAL;
    }

    int r;
    pthread_mutex_lock(&cache->mutex);
    if (consume) {
        r = move_file(file_path, cache->dir_path);
    } else {
        r = copy_file(file_path, cache->dir_path);
    }
    pthread_mutex_unlock(&cache->mutex);
    return r;
}

int fcache_add_new(fcache_t *cache, char *file_name, uint8_t *file_data, uint32_t file_data_len)
{
    if (!cache || !file_name || !file_data || !file_data_len) {
        return -EINVAL;
    }

    char buffer[PATH_MAX];
    int r = path_join(cache->dir_path, file_name, buffer, PATH_MAX);
    if (r < 0) {
        return r;
    }

    pthread_mutex_lock(&cache->mutex);
    FILE *fp = fopen(buffer, "wb");
    if (fp != NULL) {
        fwrite(file_data, file_data_len, 1, fp);
        fclose(fp);
    }
    pthread_mutex_unlock(&cache->mutex);
    return r;
}

int fcache_delete(fcache_t *cache, char *file_name)
{
    if (!cache || !file_name) {
        return -EINVAL;
    }

    char buffer[PATH_MAX];
    int r = path_join(cache->dir_path, file_name, buffer, PATH_MAX);
    if (r < 0) {
        return r;
    }

    pthread_mutex_lock(&cache->mutex);
    r = remove(buffer);
    if (r == -1)
        r = -errno;
    pthread_mutex_unlock(&cache->mutex);
    return r;
}

int fcache_copy(fcache_t *cache, char *file_name, char *dest_dir)
{
    if (!cache || !file_name || !is_dir(dest_dir)) {
        return -EINVAL;
    }

    char buffer[PATH_MAX];
    int r = path_join(cache->dir_path, file_name, buffer, PATH_MAX);
    if (r < 0) {
        return r;
    }

    pthread_mutex_lock(&cache->mutex);
    r = copy_file(buffer, dest_dir);
    pthread_mutex_unlock(&cache->mutex);
    return r;
}

int fcache_move(fcache_t *cache, char *file_name, char *dest_dir)
{
    if (!cache || !file_name || !is_dir(dest_dir)) {
        return -EINVAL;
    }

    char buffer[PATH_MAX];
    int r = path_join(cache->dir_path, file_name, buffer, PATH_MAX);
    if (r < 0) {
        return r;
    }

    pthread_mutex_lock(&cache->mutex);
    r = move_file(buffer, dest_dir);
    pthread_mutex_unlock(&cache->mutex);
    return r;
}

uint32_t fcache_size(fcache_t *cache)
{
    if (!cache) {
        return 0;
    }

    uint32_t count = 0;
    pthread_mutex_lock(&cache->mutex);
    count = files_in_dir(cache->dir_path);
    pthread_mutex_unlock(&cache->mutex);
    return count;
}

bool fcache_file_exist(fcache_t *cache, char *file_name)
{
    if (!cache || !file_name) {
        return -EINVAL;
    }

    bool found;
    pthread_mutex_lock(&cache->mutex);
    found = is_file_in_dir(cache->dir_path, file_name);
    pthread_mutex_unlock(&cache->mutex);
    return found;
}

// this could be done with cJSON, but it is a one-off
char* fcache_list_files_as_json(fcache_t *cache)
{
    if (!cache) {
        return NULL;
    }

    unsigned int size = 256;
    unsigned int offset = 0;
    char *buf = (char *)malloc(size);
    if (!buf) {
        return NULL;
    }
    buf[0] = '[';
    buf[1] = '\0';
    offset = 1;

    pthread_mutex_lock(&cache->mutex);
    DIR *d = opendir(cache->dir_path);
    if (d == NULL) {
        return NULL;
    }

    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) { // directory found
        if (strncmp(dir->d_name, ".", strlen(dir->d_name)) == 0
            || strncmp(dir->d_name, "..", strlen(dir->d_name)) == 0) {
            continue; // skip . and ..
        }

        while ((offset + strlen(dir->d_name) + 5) > size) {
            void *tmp = realloc(buf, size * 2);
            if (tmp == NULL) {
                free(buf);
                buf = NULL;
                break;
            }
            buf = tmp;
            size *= 2;

            if (size > 10024) { // hard limit
                free(buf);
                buf = NULL;
                break;
            }
        }

        if (offset == 1) { // first file name
            buf[1] = '"';
            offset++;
            buf[offset] = '\0';
        } else {
            strncat(buf, ", \"", strlen(", \"") + 1);
            offset += 3;
        }
        strncat(buf, dir->d_name, strlen(dir->d_name));
        offset += strlen(dir->d_name);
        buf[offset] = '"';
        offset++;
        buf[offset] = '\0';
    }
    closedir(d);
    pthread_mutex_unlock(&cache->mutex);

    if (buf != NULL) {
        buf[offset] = ']';
        offset++;
        buf[offset] = '\0';
        offset++;
    }
    return buf;
}
