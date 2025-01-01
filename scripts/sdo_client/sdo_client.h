#ifndef _clientLIENT_H_
#define _clientLIENT_H_

#include <stdint.h>
#include "301/CO_SDOclient.h"

char* get_sdo_abort_string(uint32_t code);

CO_SDO_abortCode_t sdo_read(CO_SDOclient_t* client, uint8_t node_id, uint16_t index, uint8_t subindex, void *buf, size_t buf_size, size_t* read_size);

CO_SDO_abortCode_t sdo_read_dynamic(CO_SDOclient_t* client, uint8_t node_id, uint16_t index, uint8_t subindex, void **buf, size_t *buf_size); // buf must be freed if not NULL

static inline CO_SDO_abortCode_t sdo_read_bool(CO_SDOclient_t* client, uint8_t node_id, uint16_t index, uint8_t subindex, bool* value) {
    return sdo_read(client, node_id, index, subindex, value, 1, NULL);
}

static inline CO_SDO_abortCode_t sdo_read_uint8(CO_SDOclient_t* client, uint8_t node_id, uint16_t index, uint8_t subindex, uint8_t* value) {
    return sdo_read(client, node_id, index, subindex, value, 1, NULL);
}

static inline CO_SDO_abortCode_t sdo_read_uint16(CO_SDOclient_t* client, uint8_t node_id, uint16_t index, uint8_t subindex, uint16_t* value) {
    return sdo_read(client, node_id, index, subindex, value, 2, NULL);
}

static inline CO_SDO_abortCode_t sdo_read_uint32(CO_SDOclient_t* client, uint8_t node_id, uint16_t index, uint8_t subindex, uint32_t* value) {
    return sdo_read(client, node_id, index, subindex, value, 4, NULL);
}

static inline CO_SDO_abortCode_t sdo_read_uint64(CO_SDOclient_t* client, uint8_t node_id, uint16_t index, uint8_t subindex, uint64_t* value) {
    return sdo_read(client, node_id, index, subindex, value, 8, NULL);
}

static inline CO_SDO_abortCode_t sdo_read_int8(CO_SDOclient_t* client, uint8_t node_id, uint16_t index, uint8_t subindex, int8_t* value) {
    return sdo_read(client, node_id, index, subindex, value, 1, NULL);
}

static inline CO_SDO_abortCode_t sdo_read_int16(CO_SDOclient_t* client, uint8_t node_id, uint16_t index, uint8_t subindex, int16_t* value) {
    return sdo_read(client, node_id, index, subindex, value, 2, NULL);
}

static inline CO_SDO_abortCode_t sdo_read_int32(CO_SDOclient_t* client, uint8_t node_id, uint16_t index, uint8_t subindex, int32_t* value) {
    return sdo_read(client, node_id, index, subindex, value, 4, NULL);
}

static inline CO_SDO_abortCode_t sdo_read_int64(CO_SDOclient_t* client, uint8_t node_id, uint16_t index, uint8_t subindex, int64_t* value) {
    return sdo_read(client, node_id, index, subindex, value, 8, NULL);
}

static inline CO_SDO_abortCode_t sdo_read_float32(CO_SDOclient_t* client, uint8_t node_id, uint16_t index, uint8_t subindex, float* value) {
    return sdo_read(client, node_id, index, subindex, value, 4, NULL);
}

static inline CO_SDO_abortCode_t sdo_read_float64(CO_SDOclient_t* client, uint8_t node_id, uint16_t index, uint8_t subindex, double* value) {
    return sdo_read(client, node_id, index, subindex, value, 8, NULL);
}

// buf must be freed if not NULL
static inline CO_SDO_abortCode_t sdo_read_bytes(CO_SDOclient_t* client, uint8_t node_id, uint16_t index, uint8_t subindex, uint8_t** buf, size_t *buf_size) {
    return sdo_read_dynamic(client, node_id, index, subindex, (void **)buf, buf_size);
}

// buf must be freed if not NULL
static inline CO_SDO_abortCode_t sdo_read_str(CO_SDOclient_t* client, uint8_t node_id, uint16_t index, uint8_t subindex, char** buf) {
    return sdo_read_dynamic(client, node_id, index, subindex, (void **)buf, NULL);
}

CO_SDO_abortCode_t sdo_write(CO_SDOclient_t* client, uint8_t node_id, uint16_t index, uint8_t subindex, void *data, size_t data_size);

static inline CO_SDO_abortCode_t sdo_write_bool(CO_SDOclient_t* client, uint8_t node_id, uint16_t index, uint8_t subindex, bool *value) {
    return sdo_write(client, node_id, index, subindex, value, 1);
}

static inline CO_SDO_abortCode_t sdo_write_uint8(CO_SDOclient_t* client, uint8_t node_id, uint16_t index, uint8_t subindex, uint8_t *value) {
    return sdo_write(client, node_id, index, subindex, value, 1);
}

static inline CO_SDO_abortCode_t sdo_write_uint16(CO_SDOclient_t* client, uint8_t node_id, uint16_t index, uint8_t subindex, uint16_t *value) {
    return sdo_write(client, node_id, index, subindex, value, 2);
}

static inline CO_SDO_abortCode_t sdo_write_uint32(CO_SDOclient_t* client, uint8_t node_id, uint16_t index, uint8_t subindex, uint32_t *value) {
    return sdo_write(client, node_id, index, subindex, value, 4);
}

static inline CO_SDO_abortCode_t sdo_write_uint64(CO_SDOclient_t* client, uint8_t node_id, uint16_t index, uint8_t subindex, uint64_t *value) {
    return sdo_write(client, node_id, index, subindex, value, 8);
}

static inline CO_SDO_abortCode_t sdo_write_int8(CO_SDOclient_t* client, uint8_t node_id, uint16_t index, uint8_t subindex, int8_t *value) {
    return sdo_write(client, node_id, index, subindex, value, 1);
}

static inline CO_SDO_abortCode_t sdo_write_int16(CO_SDOclient_t* client, uint8_t node_id, uint16_t index, uint8_t subindex, int16_t *value) {
    return sdo_write(client, node_id, index, subindex, value, 2);
}

static inline CO_SDO_abortCode_t sdo_write_int32(CO_SDOclient_t* client, uint8_t node_id, uint16_t index, uint8_t subindex, int32_t *value) {
    return sdo_write(client, node_id, index, subindex, value, 4);
}

static inline CO_SDO_abortCode_t sdo_write_int64(CO_SDOclient_t* client, uint8_t node_id, uint16_t index, uint8_t subindex, int64_t *value) {
    return sdo_write(client, node_id, index, subindex, value, 8);
}

static inline CO_SDO_abortCode_t sdo_write_bytes(CO_SDOclient_t* client, uint8_t node_id, uint16_t index, uint8_t subindex, uint8_t *buf, size_t buf_size) {
    return sdo_write(client, node_id, index, subindex, buf, buf_size);
}

static inline CO_SDO_abortCode_t sdo_write_str(CO_SDOclient_t* client, uint8_t node_id, uint16_t index, uint8_t subindex, char *buf) {
    return sdo_write(client, node_id, index, subindex, buf, strlen(buf) + 1);
}

CO_SDO_abortCode_t sdo_read_to_file(CO_SDOclient_t* client, uint8_t node_id, uint16_t index, uint8_t subindex, char *file_path);

CO_SDO_abortCode_t sdo_write_from_file(CO_SDOclient_t* client, uint8_t node_id, uint16_t index, uint8_t subindex, char *file_path);

#endif
