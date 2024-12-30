#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include "system.h"

void sleep_ms(uint32_t ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

void sleep_us(uint64_t us) {
    struct timespec ts;
    ts.tv_sec = us / 1000000;
    ts.tv_nsec = (us % 1000000) * 1000;
    nanosleep(&ts, NULL);
}

uint32_t get_uptime(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts); // monotonic is uptime on linux
    return ts.tv_sec;
}

uint32_t get_uptime_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts); // monotonic is uptime on linux
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

uint64_t get_uptime_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts); // monotonic is uptime on linux
    return (uint64_t)ts.tv_sec * 1000000 + (uint64_t)ts.tv_nsec / 1000;
}

uint32_t get_unix_time_s(void) {
    time_t rawtime;
    time(&rawtime);
    return (uint32_t)rawtime;
}

int run_bash_command(char *in, char *out, int out_max_len) {
    int out_len = 0;
    FILE *pipe = popen(in, "r");
    if (pipe == NULL) {
        return -errno;
    }
    int c;
    for (out_len = 0; (c = fgetc(pipe)) != EOF; out_len++) {
        if (out_len > (out_max_len - 1)) {
            out[out_len] = '\0';
            break; // max len reached
        }
        out[out_len] = (char)c;
    }
    pclose(pipe);
    return out_len;
}

bool is_file(const char *path) {
    bool r = false;
    FILE *fptr = fopen(path, "r");
    if (fptr != NULL) {
        fclose(fptr);
        r = true;
    }
    return r;
}

bool is_file_in_dir(const char *dir_path, const char *file_name) {
    bool found = false;
    DIR *d = opendir(dir_path);
    if (d == NULL) { // add all existing file to list
        return found;
    }

    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) { // directory found
        if (strncmp(dir->d_name, ".", strlen(dir->d_name)) == 0
            || strncmp(dir->d_name, "..", strlen(dir->d_name)) == 0) {
            continue; // skip . and ..
        }
        if (strncmp(dir->d_name, file_name, strlen(dir->d_name)) == 0) {
            found = true;
            break;
        }
    }
    closedir(d);
    return found;
}

int copy_file(const char *src, const char *dest) {
    int r = 0;
    int src_fd = open(src, O_RDONLY);
    if (src_fd == -1) {
        return -errno;
    }
    int dest_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_fd == -1) {
        close(src_fd);
        return -errno;
    }
    r = ioctl(dest_fd, FICLONE, src_fd);
    close(src_fd);
    close(dest_fd);
    return r;
}

int move_file(const char *src, const char *dest) {
    int r = copy_file(src, dest);
    if (r >= 0) {
        r = remove(src);
        if (r == -1) r = -errno;
    }
    return r;
}

bool is_dir(const char *path) {
    bool r = false;
    DIR *dir = opendir(path);
    if (dir != NULL) {
        closedir(dir);
        r = true;
    }
    return r;
}

int mkdir_path(const char *path, mode_t mode) {
    char temp_path[PATH_MAX];
    int r = 0;

    if (path == NULL) {
        return -EINVAL;
    }

    // start on 1 to skip 1st '/' in absolut path
    for (size_t i = 1; i <= strlen(path) && i < PATH_MAX - 1; ++i) {
        if (path[i] == '/' || i == strlen(path)) {
            strncpy(temp_path, path, i);
            temp_path[i] = '\0';

            if (is_dir(temp_path)) {
                continue; // dir already exist
            }

            if ((r = mkdir(temp_path, mode)) != 0) {
                break;
            }
        }
    }

    return r;
}

int clear_dir(const char *path) {
    DIR *d = opendir(path);
    if (d == NULL) { // add all existing file to list
        return -ENOENT;
    }

    int r = 0;
    char filepath[PATH_MAX];
    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) { // directory found
        if (strncmp(dir->d_name, ".", strlen(dir->d_name)) == 0
            || strncmp(dir->d_name, "..", strlen(dir->d_name)) == 0) {
            continue; // skip . and ..
        }

        if (path[strlen(path) - 1] == '/') { // path ends with a '/'
            sprintf(filepath, "%s%s", path, dir->d_name);
        } else { // need a '/' at end of path
            sprintf(filepath, "%s/%s", path, dir->d_name);
        }

        if ((r = remove(filepath)) != 0) {
            break; // remove failed
        }
    }
    closedir(d);
    return r;
}

int files_in_dir(const char *path) {
    DIR *d = opendir(path);
    if (d == NULL) {
        return -ENOENT;
    }

    int count = 0;
    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) { // directory found
        if (strncmp(dir->d_name, ".", strlen(dir->d_name)) == 0
            || strncmp(dir->d_name, "..", strlen(dir->d_name)) == 0) {
            continue; // skip . and ..
        }
        count++;
    }
    closedir(d);
    return count;
}

int path_join(const char *head, const char *tail, char *out, uint32_t out_len) {
    size_t head_len = strlen(head);
    size_t tail_len = strlen(tail);
    bool head_startswith_slash = head[0] == '/';
    bool tail_startswith_slash = tail[0] == '/';
    bool head_is_root = (head_len == 1) && head_startswith_slash;
    bool tail_is_root = (tail_len == 1) && tail_startswith_slash;

    if (tail_is_root || (head_is_root && out_len < 2)) {
        return -EINVAL;
    }

    int r = 0;
    if (head_is_root && tail_is_root) {
        out[0] = '/';
        out[1] = '\0';
    } else if (head_is_root) {
        if (tail_startswith_slash) {
            sprintf(out, "%s", tail);
        } else{
            sprintf(out, "/%s", tail);
        }
    } else {
        strncpy(out, head, head_len);
        if (out[head_len] == '/') {
            out[head_len] = '\0';
        } else {
            out[head_len] = '/';
            out[head_len + 1] = '\0';
        }
        strncat(out, tail, tail_len);
        if (out[strlen(out)] == '/') {
            out[strlen(out)] = '\0';
        }
    }

    return r;
}
