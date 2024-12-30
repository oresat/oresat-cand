#include "CANopen.h"
#include "OD.h"
#include "ecss_time.h"
#include "ecss_time_ext.h"

static ODR_t ecss_scet_read(OD_stream_t* stream, void* buf, OD_size_t count, OD_size_t* countRead) {
    (void)count;
    stream->dataLength = 8;
    get_ecss_scet((ecss_scet_t *)buf);
    *countRead = stream->dataLength;
    return ODR_OK;
}

static ODR_t ecss_scet_write(OD_stream_t* stream, const void* buf, OD_size_t count, OD_size_t* countWritten) {
    (void)count;
    set_ecss_scet((ecss_scet_t *)buf);
    *countWritten = stream->dataLength;
    return ODR_OK;
}

static ODR_t ecss_utc_read(OD_stream_t* stream, void* buf, OD_size_t count, OD_size_t* countRead) {
    (void)count;
    stream->dataLength = 8;
    get_ecss_utc((ecss_utc_t *)buf);
    *countRead = stream->dataLength;
    return ODR_OK;
}

static ODR_t ecss_utc_write(OD_stream_t* stream, const void* buf, OD_size_t count, OD_size_t* countWritten) {
    (void)count;
    set_ecss_utc((ecss_utc_t *)buf);
    *countWritten = stream->dataLength;
    return ODR_OK;
}

static OD_extension_t scet_ext = {
    .object = NULL,
    .read = ecss_scet_read,
    .write = ecss_scet_write,
};

static OD_extension_t utc_ext = {
    .object = NULL,
    .read = ecss_utc_read,
    .write = ecss_utc_write,
};

void ecss_time_extension_init(void) {
    OD_extension_init(OD_ENTRY_H2010, &scet_ext);
    OD_extension_init(OD_ENTRY_H2011, &utc_ext);
}
