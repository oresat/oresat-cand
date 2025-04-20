#ifndef _FILE_TRANSFER_EXT_H_
#define _FILE_TRANSFER_EXT_H_

#include "301/CO_ODinterface.h"
#include "fcache.h"

void file_transfer_extension_init(OD_t *od, fcache_t *fread_cache, fcache_t *fwrite_cache);
void file_transfer_extension_free(void);

#endif
