#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include "CANopen.h"
#include "301/CO_SDOclient.h"
#include "sdo_client.h"

extern CO_t* CO;
extern CO_SDOclient_t* SDO_C;

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
    "No data available",
};

char* get_abort_string(uint32_t code) {
    char *r = NULL;
    for (int i=0; i<(int)ABORT_CODES_LEN; i++) {
        if (code == ABORT_CODES[i]) {
            r = ABORT_CODES_STR[i];
        }
    }
    return r;
}

CO_SDO_abortCode_t
read_SDO(CO_SDOclient_t* SDO_C, uint8_t nodeId, uint16_t index, uint8_t subIndex, uint8_t* buf, size_t bufSize,
         size_t* readSize) {
    CO_SDO_return_t SDO_ret;

    // setup client (this can be skipped, if remote device don't change)
    SDO_ret = CO_SDOclient_setup(SDO_C, CO_CAN_ID_SDO_CLI + nodeId, CO_CAN_ID_SDO_SRV + nodeId, nodeId);
    if (SDO_ret != CO_SDO_RT_ok_communicationEnd) {
        return CO_SDO_AB_GENERAL;
    }

    // initiate upload
    SDO_ret = CO_SDOclientUploadInitiate(SDO_C, index, subIndex, 1000, false);
    if (SDO_ret != CO_SDO_RT_ok_communicationEnd) {
        return CO_SDO_AB_GENERAL;
    }

    // upload data
    do {
        uint32_t timeDifference_us = 10000;
        CO_SDO_abortCode_t abortCode = CO_SDO_AB_NONE;

        SDO_ret = CO_SDOclientUpload(SDO_C, timeDifference_us, false, &abortCode, NULL, NULL, NULL);
        if (SDO_ret < 0) {
            return abortCode;
        }

        usleep(timeDifference_us);
    } while (SDO_ret > 0);

    // copy data to the user buffer (for long data function must be called several times inside the loop)
    *readSize = CO_SDOclientUploadBufRead(SDO_C, buf, bufSize);

    return CO_SDO_AB_NONE;
}

CO_SDO_abortCode_t
write_SDO(CO_SDOclient_t* SDO_C, uint8_t nodeId, uint16_t index, uint8_t subIndex, uint8_t* data, size_t dataSize) {
    CO_SDO_return_t SDO_ret;
    bool_t bufferPartial = false;

    // setup client (this can be skipped, if remote device is the same)
    SDO_ret = CO_SDOclient_setup(SDO_C, CO_CAN_ID_SDO_CLI + nodeId, CO_CAN_ID_SDO_SRV + nodeId, nodeId);
    if (SDO_ret != CO_SDO_RT_ok_communicationEnd) {
        return -1;
    }

    // initiate download
    SDO_ret = CO_SDOclientDownloadInitiate(SDO_C, index, subIndex, dataSize, 1000, false);
    if (SDO_ret != CO_SDO_RT_ok_communicationEnd) {
        return -1;
    }

    // fill data
    size_t nWritten = CO_SDOclientDownloadBufWrite(SDO_C, data, dataSize);
    if (nWritten < dataSize) {
        bufferPartial = true;
        // If SDO Fifo buffer is too small, data can be refilled in the loop.
    }

    // download data
    do {
        uint32_t timeDifference_us = 10000;
        CO_SDO_abortCode_t abortCode = CO_SDO_AB_NONE;

        SDO_ret = CO_SDOclientDownload(SDO_C, timeDifference_us, false, bufferPartial, &abortCode, NULL, NULL);
        if (SDO_ret < 0) {
            return abortCode;
        }

        usleep(timeDifference_us);
    } while (SDO_ret > 0);

    return CO_SDO_AB_NONE;
}
