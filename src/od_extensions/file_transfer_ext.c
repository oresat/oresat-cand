#include "file_transfer_ext.h"
#include "301/CO_ODinterface.h"
#include "OD.h"
#include "fcache.h"
#include "logger.h"
#include "od_ext.h"
#include "system.h"
#include <libgen.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#define PATH_LEN 256 // PATH_MAX (4096) is a little much

typedef struct {
    char name[10];
    char raw[PATH_LEN];           // raw file name written over CAN (can be a name or path)
    char file_name[PATH_LEN];     // file basename
    char file_path[PATH_LEN];     // the src/dest file path
    char tmp_file_path[PATH_LEN]; // the /tmp file path using during CAN fread/fwrites
    FILE *fp;
    fcache_t *cache;  // file cache to use if no path is given
    bool file_cached; // convience flag for if file src/dest is the cache
    char *files;
} file_transfer_data_t;

static ODR_t file_transfer_read(OD_stream_t *stream, void *buf, OD_size_t count, OD_size_t *countRead);
static ODR_t file_transfer_write(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten);

static OD_extension_t fread_ext = {
    .object = NULL,
    .read = file_transfer_read,
    .write = file_transfer_write,
};

static OD_extension_t fwrite_ext = {
    .object = NULL,
    .read = file_transfer_read,
    .write = file_transfer_write,
};

void file_transfer_extension_init(OD_t *od, fcache_t *fread_cache, fcache_t *fwrite_cache) {
    OD_entry_t *entry;

    entry = OD_find(od, OD_INDEX_FREAD_CACHE);
    if (entry != NULL) {
        file_transfer_data_t *fread_data = malloc(sizeof(file_transfer_data_t));
        if (fread_data != NULL) {
            strcpy(fread_data->name, "fread");
            fread_data->fp = NULL;
            fread_data->file_cached = false;
            fread_data->raw[0] = '\0';
            fread_data->file_name[0] = '\0';
            fread_data->file_path[0] = '\0';
            fread_data->tmp_file_path[0] = '\0';
            fread_data->cache = fread_cache;
            fread_data->files = NULL;
        }
        fread_ext.object = fread_data;
        OD_extension_init(entry, &fread_ext);
    } else {
        log_critical("could not find fread cache enty 0x(%X)", OD_INDEX_FREAD_CACHE);
    }

    entry = OD_find(od, OD_INDEX_FWRITE_CACHE);
    if (entry != NULL) {
        file_transfer_data_t *fwrite_data = malloc(sizeof(file_transfer_data_t));
        if (fwrite_data != NULL) {
            strcpy(fwrite_data->name, "fwrite");
            fwrite_data->fp = NULL;
            fwrite_data->file_cached = false;
            fwrite_data->raw[0] = '\0';
            fwrite_data->file_name[0] = '\0';
            fwrite_data->file_path[0] = '\0';
            fwrite_data->tmp_file_path[0] = '\0';
            fwrite_data->cache = fwrite_cache;
            fwrite_data->files = NULL;
        }
        fwrite_ext.object = fwrite_data;
        OD_extension_init(entry, &fwrite_ext);
    } else {
        log_critical("could not find fwrite cache enty 0x(%X)", OD_INDEX_FWRITE_CACHE);
    }
}

void file_transfer_extension_free(void) {
    file_transfer_data_t *fdata;

    if (fread_ext.object != NULL) {
        fdata = (file_transfer_data_t *)fread_ext.object;
        if (fdata->fp) {
            fclose(fdata->fp);
        }
        if (fdata->files) {
            free(fdata->files);
        }
        free(fread_ext.object);
        fread_ext.object = NULL;
    }

    if (fwrite_ext.object != NULL) {
        fdata = (file_transfer_data_t *)fwrite_ext.object;
        if (fdata->fp) {
            fclose(fdata->fp);
        }
        if (fdata->files) {
            free(fdata->files);
        }
        free(fwrite_ext.object);
        fwrite_ext.object = NULL;
    }
}

static ODR_t file_transfer_read(OD_stream_t *stream, void *buf, OD_size_t count, OD_size_t *countRead) {
    file_transfer_data_t *fdata = (file_transfer_data_t *)stream->object;
    if (fdata == NULL) {
        log_error("no cache data");
        return ODR_DEV_INCOMPAT;
    }

    int e;
    ODR_t r = ODR_OK;
    switch (stream->subIndex) {
    case 0:
        r = OD_readOriginal(stream, buf, count, countRead);
        break;
    case OD_SUBINDEX_FREAD_CACHE_LENGTH: {
        uint8_t size = MIN(fcache_size(fdata->cache), 0xFF);
        *countRead = sizeof(size);
        memcpy(buf, &size, *countRead);
        break;
    }
    case OD_SUBINDEX_FREAD_CACHE_FILES_JSON: {
        if ((stream->dataOffset == 0) && (fdata->files != NULL)) {
            free(fdata->files);
        }
        fdata->files = fcache_list_files_as_json(fdata->cache);
        if (fdata->files != NULL) {
            r = od_ext_read_data(stream, buf, count, countRead, fdata->files, strlen(fdata->files) + 1);
        }
        break;
    }
    case OD_SUBINDEX_FREAD_CACHE_FILE_NAME:
        r = od_ext_read_data(stream, buf, count, countRead, fdata->raw, strlen(fdata->raw) + 1);
        break;
    case OD_SUBINDEX_FREAD_CACHE_FILE_DATA:
        if (fdata->file_name[0] == '\0') {
            log_error("%s cache file name subindex must be set first", fdata->name);
            r = ODR_NO_DATA;
        } else {
            if (stream->dataOffset == 0) {
                if (!is_file(fdata->file_path)) {
                    log_error("%s cache %s does not exist", fdata->name, fdata->file_name);
                    r = ODR_NO_DATA;
                } else {
                    path_join("/tmp", fdata->file_name, fdata->tmp_file_path, PATH_LEN);
                    e = copy_file(fdata->file_path, fdata->tmp_file_path);
                    if (e < 0) {
                        log_error("%s cache failed to copy %s to %s", fdata->name, fdata->file_name,
                                  fdata->tmp_file_path);
                        r = ODR_DATA_LOC_CTRL;
                    }
                }
            }

            if (r == ODR_OK) {
                r = od_ext_read_file(stream, buf, count, countRead, fdata->tmp_file_path, &fdata->fp);
            }
        }
        break;
    default:
        r = ODR_WRITEONLY;
        break;
    }
    return r;
}

static ODR_t file_transfer_write(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten) {
    int e;
    file_transfer_data_t *fdata = (file_transfer_data_t *)stream->object;
    if (fdata == NULL) {
        log_error("no cache data");
        return ODR_DEV_INCOMPAT;
    }

    ODR_t r = ODR_OK;
    switch (stream->subIndex) {
    case OD_SUBINDEX_FREAD_CACHE_FILE_NAME: {
        size_t dataWritten = 0;
        r = od_ext_write_data(stream, buf, count, countWritten, fdata->raw, PATH_LEN - 1, &dataWritten);
        if (r == ODR_OK) {
            strncpy(fdata->file_name, basename(fdata->raw), strlen(basename(fdata->raw)) + 1);
            fdata->file_cached = !strncmp(fdata->raw, fdata->file_name, strlen(fdata->raw));
            if (fdata->file_cached) {
                path_join(fdata->cache->dir_path, fdata->file_name, fdata->file_path, PATH_LEN);
            } else {
                strncpy(fdata->file_path, fdata->raw, strlen(fdata->raw) + 1);
            }
        }
        break;
    }
    case OD_SUBINDEX_FREAD_CACHE_FILE_DATA:
        if (fdata->file_name[0] == '\0') {
            log_error("%s cache file name subindex must be set first", fdata->name);
            return ODR_NO_DATA; // file name must be set first
        }

        if (stream->dataOffset == 0) {
            path_join("/tmp", fdata->file_name, fdata->tmp_file_path, PATH_LEN);
        }
        r = od_ext_write_file(stream, buf, count, countWritten, fdata->tmp_file_path, &fdata->fp);
        if (r == ODR_OK) {
            if (fdata->raw[0] == '/') {
                if (!strncmp(fdata->raw, fdata->tmp_file_path, strlen(fdata->raw) + 1)) {
                    e = move_file(fdata->tmp_file_path, fdata->raw);
                    if (e < 0) {
                        log_error("failed to move %s to %s: %d", fdata->tmp_file_path, fdata->file_name, r);
                    }
                }
            } else {
                e = fcache_add(fdata->cache, fdata->tmp_file_path, true);
                if (e < 0) {
                    log_error("failed to move %s to %s cache: %d", fdata->tmp_file_path, fdata->name, r);
                }
            }
        }
        break;
    case OD_SUBINDEX_FREAD_CACHE_REMOVE:
        if ((bool *)buf && (fdata->file_name[0] != '\0')) {
            fcache_delete(fdata->cache, fdata->file_name);
            remove(fdata->tmp_file_path);
            log_info("deleted %s", fdata->file_name);
            fdata->raw[0] = '\0';
            fdata->file_name[0] = '\0';
            fdata->file_path[0] = '\0';
            fdata->tmp_file_path[0] = '\0';
        }
        *countWritten = 1;
        break;
    default:
        r = ODR_READONLY;
        break;
    }
    return r;
}
