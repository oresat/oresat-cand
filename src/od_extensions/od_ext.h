#ifndef _OD_EXT_H_
#define _OD_EXT_H_

#include "301/CO_ODinterface.h"

#define SDO_BLOCK_LEN  (127 * 7)

ODR_t od_ext_read_data(OD_stream_t* stream, void* buf, OD_size_t count, OD_size_t* countRead, void *data, size_t dataLen);

ODR_t od_ext_write_data(OD_stream_t* stream, const void* buf, OD_size_t count, OD_size_t* countWritten, void *data, size_t dataMaxLen, size_t *dataWritten);

#endif
