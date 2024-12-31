#include <libgen.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include "301/CO_ODinterface.h"
#include "system.h"
#include "fcache.h"
#include "od_ext.h"
#include "file_transfer_ext.h"

#define PATH_LEN 256 // PATH_MAX (4096) is a little much

typedef struct {
    char raw[PATH_LEN];             // raw file name written over CAN (can be a name or path)
    char file_name[PATH_LEN];       // file basename
    char file_path[PATH_LEN];       // the src/dest file path
    char tmp_file_path[PATH_LEN];   // the /tmp file path using during CAN fread/fwrites
    FILE *fp;
    fcache_t *cache;                // file cache to use if no path is given
    bool file_cached;               // convience flag for if file src/dest is the cache
} file_transfer_data_t;

static ODR_t file_transfer_read(OD_stream_t* stream, void* buf, OD_size_t count, OD_size_t* countRead);
static ODR_t file_transfer_write(OD_stream_t* stream, const void* buf, OD_size_t count, OD_size_t* countWritten);

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
    OD_entry_t* entry;

    entry = OD_find(od, FREAD_CACHE_INDEX);
    if (entry != NULL) {
        file_transfer_data_t *fread_data = malloc(sizeof(file_transfer_data_t));
        if (fread_data != NULL) {
            fread_data->file_cached = false;
            fread_data->raw[0] = '\0';
            fread_data->file_name[0] = '\0';
            fread_data->file_path[0] = '\0';
            fread_data->tmp_file_path[0] = '\0';
            fread_data->cache = fread_cache;
        }
        fread_ext.object = fread_data;
        OD_extension_init(entry, &fread_ext);
    }

    entry = OD_find(od, FWRITE_CACHE_INDEX);
    if (entry != NULL) {
        file_transfer_data_t *fwrite_data = malloc(sizeof(file_transfer_data_t));
        if (fwrite_data != NULL) {
            fwrite_data->file_cached = false;
            fwrite_data->raw[0] = '\0';
            fwrite_data->file_name[0] = '\0';
            fwrite_data->file_path[0] = '\0';
            fwrite_data->tmp_file_path[0] = '\0';
            fwrite_data->cache = fwrite_cache;
        }
        fwrite_ext.object = fwrite_data;
        OD_extension_init(entry, &fwrite_ext);
    }
}

void file_transfer_extension_free(void) {
    file_transfer_data_t *fdata;

    if (fread_ext.object != NULL) {
        fdata = (file_transfer_data_t *)fread_ext.object;
        if (fdata->fp) {
            fclose(fdata->fp);
        }
        free(fread_ext.object);
        fread_ext.object = NULL;
    }

    if (fwrite_ext.object != NULL) {
        fdata = (file_transfer_data_t *)fwrite_ext.object;
        if (fdata->fp) {
            fclose(fdata->fp);
        }
        free(fwrite_ext.object);
        fwrite_ext.object = NULL;
    }
}

static ODR_t file_transfer_read(OD_stream_t* stream, void* buf, OD_size_t count, OD_size_t* countRead) {
    file_transfer_data_t *fdata = (file_transfer_data_t *)stream->object;
    if (fdata == NULL) {
        return ODR_DEV_INCOMPAT;
    }

    ODR_t r = ODR_OK;
    switch (stream->subIndex) {
        case 0:
            r = OD_readOriginal(stream, buf, count, countRead);
            break;
        case FILE_TRANSFER_SUBINDEX_LENGTH:
        {
            uint8_t size = MIN(fcache_size(fdata->cache), 0xFF);
            *countRead = sizeof(size);
            memcpy(buf, &size, *countRead);
            break;
        }
        case FILE_TRANSFER_SUBINDEX_FILES:
        {
            char data[] = "[]"; // TODO
            *countRead = strlen(data) + 1;
            strncpy(buf, data, *countRead);
            break;
        }
        case FILE_TRANSFER_SUBINDEX_NAME:
            r = od_ext_read_data(stream, buf, count, countRead, fdata->raw, strlen(fdata->raw) + 1);
            break;
        case FILE_TRANSFER_SUBINDEX_DATA:
            if (fdata->file_name[0] == '\0') {
                r = ODR_NO_DATA; // file name must be set first
            } else {
                if (stream->dataOffset == 0) {
                    if (!is_file(fdata->file_path)) {
                        r = ODR_NO_DATA;
                    } else {
                        path_join("/tmp", fdata->file_name,  fdata->tmp_file_path, PATH_LEN);
                        copy_file(fdata->file_path, fdata->tmp_file_path);
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

static ODR_t file_transfer_write(OD_stream_t* stream, const void* buf, OD_size_t count, OD_size_t* countWritten) {
    file_transfer_data_t *fdata = (file_transfer_data_t *)stream->object;
    if (fdata == NULL) {
        return ODR_DEV_INCOMPAT;
    }

    ODR_t r = ODR_OK;
    switch (stream->subIndex) {
        case FILE_TRANSFER_SUBINDEX_NAME:
        {
            size_t dataWritten = 0;
            r = od_ext_write_data(stream, buf, count, countWritten, fdata->raw, PATH_LEN - 1, &dataWritten);
            if (r == ODR_OK) {
                strncpy(fdata->file_name, basename(fdata->raw), strlen(basename(fdata->raw)) + 1);
                fdata->file_cached = !strncmp(fdata->raw, fdata->file_name, strlen(fdata->raw));
                if (fdata->file_cached) {
                    path_join(fdata->cache->dir_path, fdata->file_name,  fdata->file_path, PATH_LEN);
                } else {
                    strncpy(fdata->file_path, fdata->raw, strlen(fdata->raw) + 1);
                }
            }
            break;
        }
        case FILE_TRANSFER_SUBINDEX_DATA:
            if (fdata->file_name[0] == '\0') {
                return ODR_NO_DATA; // file name must be set first
            }

            if (stream->dataOffset == 0) {
                path_join("/tmp", fdata->file_name,  fdata->tmp_file_path, PATH_LEN);
            }
            r = od_ext_write_file(stream, buf, count, countWritten, fdata->tmp_file_path, &fdata->fp);
            if (r == ODR_OK) {
                fcache_add(fdata->cache, fdata->tmp_file_path, true);
            }
            break;
        case FILE_TRANSFER_SUBINDEX_DELETE:
            if ((bool *)buf && (fdata->file_name[0] != '\0')) {
                fcache_delete(fdata->cache, fdata->file_name);
                remove(fdata->tmp_file_path);
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
