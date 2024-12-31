#include <stdint.h>
#include <string.h>
#include "301/CO_ODinterface.h"
#include "od_ext.h"

ODR_t od_ext_read_data(OD_stream_t* stream, void* buf, OD_size_t count, OD_size_t* countRead, void *data, size_t dataLen) {
    if ((stream == NULL) || (buf == NULL) || (countRead == NULL)) {
        return ODR_DEV_INCOMPAT;
    }

    OD_size_t dataLenToCopy = dataLen;
    const uint8_t* dataOrig = (uint8_t *)data;

    if (dataOrig == NULL) {
        return ODR_SUB_NOT_EXIST;
    }

    ODR_t returnCode = ODR_OK;

    /* If previous read was partial or OD variable length is larger than
     * current buffer size, then data was (will be) read in several segments */
    if ((stream->dataOffset > 0U) || (dataLenToCopy > count)) {
        if (stream->dataOffset >= dataLenToCopy) {
            return ODR_DEV_INCOMPAT;
        }
        /* Reduce for already copied data */
        dataLenToCopy -= stream->dataOffset;
        dataOrig += stream->dataOffset;

        if (dataLenToCopy > count) {
            /* Not enough space in destination buffer */
            dataLenToCopy = count;
            stream->dataOffset += dataLenToCopy;
            returnCode = ODR_PARTIAL;
        } else {
            stream->dataOffset = 0; /* copy finished, reset offset */
        }
    }

    (void)memcpy((void*)buf, (const void*)dataOrig, dataLenToCopy);

    *countRead = dataLenToCopy;
    return returnCode;
}

ODR_t od_ext_write_data(OD_stream_t* stream, const void* buf, OD_size_t count, OD_size_t* countWritten, void *data, size_t dataMaxLen, size_t *dataWritten) {
    if ((stream == NULL) || (buf == NULL) || (countWritten == NULL)) {
        return ODR_DEV_INCOMPAT;
    }

    if (stream->dataLength > dataMaxLen) {
        return ODR_DEV_INCOMPAT;
    }

    OD_size_t dataLenToCopy = stream->dataLength; /* length of OD variable */
    OD_size_t dataLenRemain = dataLenToCopy;      /* remaining length of dataOrig buffer */
    uint8_t* dataOrig = (uint8_t *)data;

    if (dataOrig == NULL) {
        return ODR_SUB_NOT_EXIST;
    }

    ODR_t returnCode = ODR_OK;

    /* If previous write was partial or OD variable length is larger than current buffer size,
     * then data was (will be) written in several segments */
    if ((stream->dataOffset > 0U) || (dataLenToCopy > count)) {
        if (stream->dataOffset >= dataLenToCopy) {
            return ODR_DEV_INCOMPAT;
        }
        /* reduce for already copied data */
        dataLenToCopy -= stream->dataOffset;
        dataLenRemain = dataLenToCopy;
        dataOrig += stream->dataOffset;

        if (dataLenToCopy > count) {
            /* Remaining data space in OD variable is larger than current count
             * of data, so only current count of data will be copied */
            dataLenToCopy = count;
            stream->dataOffset += dataLenToCopy;
            returnCode = ODR_PARTIAL;
        } else {
            stream->dataOffset = 0; /* copy finished, reset offset */
        }
    }

    if (dataLenToCopy < count) {
        /* OD variable is smaller than current amount of data */
        return ODR_DATA_LONG;
    }

    /* additional check for Misra c compliance */
    if ((dataLenToCopy <= dataLenRemain) && (dataLenToCopy <= count)) {
        (void)memcpy((void*)dataOrig, (const void*)buf, dataLenToCopy);
    } else {
        return ODR_DEV_INCOMPAT;
    }

    if ((stream->dataOffset == 0) && (dataWritten != NULL)) {
        *dataWritten = stream->dataLength;
    }

    *countWritten = dataLenToCopy;
    return returnCode;
}

ODR_t od_ext_read_file(OD_stream_t* stream, void* buf, OD_size_t count, OD_size_t* countRead, const char *file_path,  FILE **fp) {
    if (!stream || !buf || !countRead || !file_path || !file_path) {
        return ODR_DEV_INCOMPAT;
    }

    if (stream->dataOffset == 0U) {
        if (*fp != NULL) {
            fclose(*fp);
        }
        *fp = fopen(file_path, "rb");
        if (*fp != NULL) {
            fseek(*fp, 0L, SEEK_END);
            stream->dataLength = ftell(*fp);
            rewind(*fp);
        } else {
            return ODR_DEV_INCOMPAT;
        }
    }

    OD_size_t dataLenToCopy = stream->dataLength;
    ODR_t returnCode = ODR_OK;

    /* If previous read was partial or OD variable length is larger than
     * current buffer size, then data was (will be) read in several segments */
    if ((stream->dataOffset > 0U) || (dataLenToCopy > count)) {
        if (stream->dataOffset >= dataLenToCopy) {
            return ODR_DEV_INCOMPAT;
        }
        /* Reduce for already copied data */
        dataLenToCopy -= stream->dataOffset;

        if (dataLenToCopy > count) {
            /* Not enough space in destination buffer */
            dataLenToCopy = count;
            stream->dataOffset += dataLenToCopy;
            returnCode = ODR_PARTIAL;
        } else {
            stream->dataOffset = 0; /* copy finished, reset offset */
        }
    }

    if (fp != NULL) {
        fread(buf, dataLenToCopy, 1, *fp);
        if (returnCode != ODR_PARTIAL) {
            fclose(*fp);
            *fp = NULL;
        }
    }

    *countRead = dataLenToCopy;
    return returnCode;
}

ODR_t od_ext_write_file(OD_stream_t* stream, const void* buf, OD_size_t count, OD_size_t* countWritten, const char *file_path, FILE **fp) {
    if (!stream || !buf || !countWritten || !file_path || !fp) {
        return ODR_DEV_INCOMPAT;
    }

    if (stream->dataOffset == 0U) {
        if (*fp != NULL) {
            fclose(*fp);
            *fp = NULL;
        }
        *fp = fopen(file_path, "wb");
        if (*fp == NULL) {
            return ODR_DEV_INCOMPAT;
        }
    }

    OD_size_t dataLenToCopy = stream->dataLength; /* length of OD variable */
    OD_size_t dataLenRemain = dataLenToCopy;      /* remaining length of file buffer */

    ODR_t returnCode = ODR_OK;

    /* If previous write was partial or OD variable length is larger than current buffer size,
     * then data was (will be) written in several segments */
    if ((stream->dataOffset > 0U) || (dataLenToCopy > count)) {
        if (stream->dataOffset >= dataLenToCopy) {
            return ODR_DEV_INCOMPAT;
        }
        /* reduce for already copied data */
        dataLenToCopy -= stream->dataOffset;
        dataLenRemain = dataLenToCopy;

        if (dataLenToCopy > count) {
            /* Remaining data space in OD variable is larger than current count
             * of data, so only current count of data will be copied */
            dataLenToCopy = count;
            stream->dataOffset += dataLenToCopy;
            returnCode = ODR_PARTIAL;
        } else {
            stream->dataOffset = 0; /* copy finished, reset offset */
        }
    }

    if (dataLenToCopy < count) {
        /* OD variable is smaller than current amount of data */
        return ODR_DATA_LONG;
    }

    /* additional check for Misra c compliance */
    if ((dataLenToCopy <= dataLenRemain) && (dataLenToCopy <= count)) {
        if (*fp != NULL) {
            fwrite(buf, dataLenToCopy, 1, *fp);
            if (returnCode != ODR_PARTIAL) {
                fclose(*fp);
                *fp = NULL;
            }
        }
    } else {
        return ODR_DEV_INCOMPAT;
    }

    *countWritten = dataLenToCopy;
    return returnCode;
}
