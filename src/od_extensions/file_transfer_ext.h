#include "301/CO_ODinterface.h"
#include "fcache.h"

#ifndef _FILE_TRANSFER_EXT_H_
#define _FILE_TRANSFER_EXT_H_

#define FREAD_CACHE_INDEX 0x3004
#define FWRITE_CACHE_INDEX 0x3005
#define FILE_TRANSFER_SUBINDEX_LENGTH 0x1
#define FILE_TRANSFER_SUBINDEX_FILES 0x2
#define FILE_TRANSFER_SUBINDEX_NAME 0x3
#define FILE_TRANSFER_SUBINDEX_DATA 0x4
#define FILE_TRANSFER_SUBINDEX_DELETE 0x5

void file_transfer_extension_init(OD_t *od, fcache_t *fread_cache, fcache_t *fwrite_cache);
void file_transfer_extension_free(void);

#endif
