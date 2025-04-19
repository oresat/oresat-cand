#include "ecss_time_ext.h"
#include "301/CO_ODinterface.h"
#include "ecss_time.h"
#include "logger.h"

static ODR_t ecss_scet_read(OD_stream_t *stream, void *buf, OD_size_t count, OD_size_t *countRead) {
    (void)count;
    stream->dataLength = 8;
    get_ecss_scet((ecss_scet_t *)buf);
    *countRead = stream->dataLength;
    return ODR_OK;
}

static ODR_t ecss_scet_write(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten) {
    (void)count;
    int r = set_ecss_scet((ecss_scet_t *)buf);
    if (r < 0) {
        log_error("cannot set system time to ecss scet: %d", -r);
    }
    *countWritten = stream->dataLength;
    return ODR_OK;
}

static ODR_t ecss_utc_read(OD_stream_t *stream, void *buf, OD_size_t count, OD_size_t *countRead) {
    (void)count;
    stream->dataLength = 8;
    get_ecss_utc((ecss_utc_t *)buf);
    *countRead = stream->dataLength;
    return ODR_OK;
}

static ODR_t ecss_utc_write(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten) {
    (void)count;
    int r = set_ecss_utc((ecss_utc_t *)buf);
    if (r < 0) {
        log_error("cannot set system time to ecss utc: %d", -r);
    }
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

void ecss_time_extension_init(OD_t *od) {
    OD_entry_t *entry;

    entry = OD_find(od, 0x2010);
    OD_extension_init(entry, &scet_ext);

    entry = OD_find(od, 0x2011);
    OD_extension_init(entry, &utc_ext);
}
