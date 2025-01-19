#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include "system.h"
#include "CO_SDOserver.h"
#include "sdo_client.h"

uint32_t ABORT_CODES[] = {
    0x00000000UL,
    0x05030000UL,
    0x05040000UL,
    0x05040001UL,
    0x05040002UL,
    0x05040003UL,
    0x05040004UL,
    0x05040005UL,
    0x06010000UL,
    0x06010001UL,
    0x06010002UL,
    0x06020000UL,
    0x06040041UL,
    0x06040042UL,
    0x06040043UL,
    0x06040047UL,
    0x06060000UL,
    0x06070010UL,
    0x06070012UL,
    0x06070013UL,
    0x06090011UL,
    0x06090030UL,
    0x06090031UL,
    0x06090032UL,
    0x06090036UL,
    0x060A0023UL,
    0x08000000UL,
    0x08000020UL,
    0x08000021UL,
    0x08000022UL,
    0x08000023UL,
    0x08000024UL,
};
#define ABORT_CODES_LEN (sizeof(ABORT_CODES)/sizeof(ABORT_CODES[0]))

char *ABORT_CODES_STR[] = {
    "No abort",
    "Toggle bit not altered",
    "SDO protocol timed out",
    "Command specifier not valid or unknown",
    "Invalid block size in block mode",
    "Invalid sequence number in block mode",
    "CRC error (block mode only)",
    "Out of memory",
    "Unsupported access to an object",
    "Attempt to read a write only object",
    "Attempt to write a read only object",
    "Object does not exist in the object dictionary",
    "Object cannot be mapped to the PDO",
    "Number and length of object to be mapped exceeds PDO length",
    "General parameter incompatibility reasons",
    "General internal incompatibility in device",
    "Access failed due to hardware error",
    "length of service parameter does not match",
    "length of service parameter too high",
    "length of service parameter too short",
    "Sub index does not exist",
    "Invalid value for parameter (download only).",
    "Value range of parameter written too high",
    "Value range of parameter written too low",
    "Maximum value is less than minimum value.",
    "Resource not available: SDO connection",
    "General error",
    "Data cannot be transferred or stored to application",
    "Data cannot be transferred or stored to application because of local control",
    "Data cannot be transferred or stored to application because of present device state",
    "Object dictionary not present or dynamic generation fails",
    "No buf available",
};

char* get_sdo_abort_string(uint32_t code) {
    char *r = NULL;
    for (int i=0; i<(int)ABORT_CODES_LEN; i++) {
        if (code == ABORT_CODES[i]) {
            r = ABORT_CODES_STR[i];
        }
    }
    return r;
}

CO_SDO_abortCode_t
sdo_read_dynamic(CO_SDOclient_t* client, uint8_t node_id, uint16_t index, uint8_t subindex, void **buf, size_t *buf_size) {
    CO_SDO_return_t ret;

    ret = CO_SDOclient_setup(client, CO_CAN_ID_SDO_CLI + node_id, CO_CAN_ID_SDO_SRV + node_id, node_id);
    if (ret != CO_SDO_RT_ok_communicationEnd) {
        return CO_SDO_AB_GENERAL;
    }

    ret = CO_SDOclientUploadInitiate(client, index, subindex, 1000, true);
    if (ret != CO_SDO_RT_ok_communicationEnd) {
        return CO_SDO_AB_GENERAL;
    }

    size_t size = 0;
    uint8_t *tmp = NULL;
    size_t offset = 0;
    size_t size_indicated = 0;
    uint32_t time_diff_us = 10000;
    CO_SDO_abortCode_t abort_code = CO_SDO_AB_NONE;
    do {

        ret = CO_SDOclientUpload(client, time_diff_us, false, &abort_code, &size_indicated, NULL, NULL);
        if (ret < 0) {
            return abort_code;
        }

        if (size_indicated != 0) {
            if (size == 0) {
                tmp = malloc(size_indicated + 1); // +1 for strings with missing '\0'
                tmp[size_indicated] = '\0';
                if (!tmp) {
                    return CO_SDO_AB_GENERAL;
                }
                size = size_indicated;
            }
            offset += CO_SDOclientUploadBufRead(client, &tmp[offset], size);
        }

        sleep_us(time_diff_us);
    } while (ret > 0);

    if (abort_code == CO_SDO_AB_NONE) {
        *buf = tmp;
        if (buf_size != NULL) {
            *buf_size = size;
        }
    } else if (tmp) {
        free(tmp);
    }
    return CO_SDO_AB_NONE;
}

CO_SDO_abortCode_t
sdo_read(CO_SDOclient_t* client, uint8_t node_id, uint16_t index, uint8_t subindex, void* buf, size_t buf_size, size_t* read_size) {
    CO_SDO_return_t ret;
    uint8_t *data = (uint8_t *)buf;

    ret = CO_SDOclient_setup(client, CO_CAN_ID_SDO_CLI + node_id, CO_CAN_ID_SDO_SRV + node_id, node_id);
    if (ret != CO_SDO_RT_ok_communicationEnd) {
        return CO_SDO_AB_GENERAL;
    }

    ret = CO_SDOclientUploadInitiate(client, index, subindex, 1000, true);
    if (ret != CO_SDO_RT_ok_communicationEnd) {
        return CO_SDO_AB_GENERAL;
    }

    size_t offset = 0;
    do {
        uint32_t time_diff_us = 10000;
        CO_SDO_abortCode_t abort_code = CO_SDO_AB_NONE;

        ret = CO_SDOclientUpload(client, time_diff_us, false, &abort_code, NULL, NULL, NULL);
        if (ret < 0) {
            return abort_code;
        }

        if (ret >= 0) {
            offset += CO_SDOclientUploadBufRead(client, &data[offset], buf_size);
        }

        sleep_us(time_diff_us);
    } while (ret > 0);

    if (read_size != NULL) {
        *read_size = offset;
    }

    return CO_SDO_AB_NONE;
}

CO_SDO_abortCode_t
sdo_write(CO_SDOclient_t* client, uint8_t node_id, uint16_t index, uint8_t subindex, void* buf, size_t buf_size) {
    CO_SDO_return_t ret;
    uint8_t *data = (uint8_t *)buf;

    ret = CO_SDOclient_setup(client, CO_CAN_ID_SDO_CLI + node_id, CO_CAN_ID_SDO_SRV + node_id, node_id);
    if (ret != CO_SDO_RT_ok_communicationEnd) {
        return CO_SDO_AB_GENERAL;
    }

    ret = CO_SDOclientDownloadInitiate(client, index, subindex, buf_size, 1000, true);
    if (ret != CO_SDO_RT_ok_communicationEnd) {
        return CO_SDO_AB_DATA_LOC_CTRL;
    }

    bool buffer_partial = true;
    uint32_t time_diff_us = 10000;
    CO_SDO_abortCode_t abort_code = CO_SDO_AB_NONE;
    size_t offset = 0;

    do {
        if (offset < buf_size) {
            offset += CO_SDOclientDownloadBufWrite(client, &data[offset], buf_size);
            buffer_partial = offset < buf_size;
        }

        ret = CO_SDOclientDownload(client, time_diff_us, false, buffer_partial, &abort_code, NULL, NULL);
        if (ret < 0) {
            return abort_code;
        }

        sleep_us(time_diff_us);
    } while (ret > 0);

    return CO_SDO_AB_NONE;
}

CO_SDO_abortCode_t
sdo_read_to_file(CO_SDOclient_t* client, uint8_t node_id, uint16_t index, uint8_t subindex, char *file_path) {
    CO_SDO_return_t ret;

    ret = CO_SDOclient_setup(client, CO_CAN_ID_SDO_CLI + node_id, CO_CAN_ID_SDO_SRV + node_id, node_id);
    if (ret != CO_SDO_RT_ok_communicationEnd) {
        return CO_SDO_AB_GENERAL;
    }

    ret = CO_SDOclientUploadInitiate(client, index, subindex, 1000, true);
    if (ret != CO_SDO_RT_ok_communicationEnd) {
        return CO_SDO_AB_GENERAL;
    }

    size_t nbytes;
    uint8_t buf[client->bufFifo.bufSize];

    FILE *fp = fopen(file_path, "wb");
    if (fp == NULL) {
        return CO_SDO_AB_GENERAL;
    }

    do {
        uint32_t time_diff_us = 10000;
        CO_SDO_abortCode_t abort_code = CO_SDO_AB_NONE;

        ret = CO_SDOclientUpload(client, time_diff_us, false, &abort_code, NULL, NULL, NULL);
        if (ret < 0) {
            fclose(fp);
            return abort_code;
        }

        if (ret >= 0) {
            nbytes = CO_SDOclientUploadBufRead(client, buf, client->bufFifo.bufSize);
            fwrite(buf, nbytes, 1, fp);
        }

        sleep_us(time_diff_us);
    } while (ret > 0);

    fclose(fp);
    return CO_SDO_AB_NONE;
}

CO_SDO_abortCode_t
sdo_write_from_file(CO_SDOclient_t* client, uint8_t node_id, uint16_t index, uint8_t subindex, char *file_path) {
    CO_SDO_return_t ret;

    ret = CO_SDOclient_setup(client, CO_CAN_ID_SDO_CLI + node_id, CO_CAN_ID_SDO_SRV + node_id, node_id);
    if (ret != CO_SDO_RT_ok_communicationEnd) {
        return CO_SDO_AB_GENERAL;
    }

    FILE *fp = fopen(file_path, "rb");
    if (fp == NULL) {
        return CO_SDO_AB_GENERAL;
    }

    fseek(fp, 0L, SEEK_END);
    size_t buf_size = ftell(fp);
    rewind(fp);

    ret = CO_SDOclientDownloadInitiate(client, index, subindex, buf_size, 1000, true);
    if (ret != CO_SDO_RT_ok_communicationEnd) {
        fclose(fp);
        return CO_SDO_AB_DATA_LOC_CTRL;
    }

    bool buffer_partial = true;
    uint32_t time_diff_us = 10000;
    CO_SDO_abortCode_t abort_code = CO_SDO_AB_NONE;
    size_t offset = 0;
    size_t space = 0;
    uint8_t buf[client->bufFifo.bufSize];

    do {
        space = CO_fifo_getSpace(&client->bufFifo);
        if ((offset < buf_size) && (space > 0)) {
            fread(buf, space, 1, fp);
            offset += CO_SDOclientDownloadBufWrite(client, buf, buf_size);
            buffer_partial = offset < buf_size;
        }

        ret = CO_SDOclientDownload(client, time_diff_us, false, buffer_partial, &abort_code, NULL, NULL);
        if (ret < 0) {
            fclose(fp);
            return abort_code;
        }

        sleep_us(time_diff_us);
    } while (ret > 0);

    fclose(fp);
    return CO_SDO_AB_NONE;
}
