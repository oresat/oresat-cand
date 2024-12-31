#ifndef _SDO_CLIENT_H_
#define _SDO_CLIENT_H_

#include <stdint.h>
#include "301/CO_SDOclient.h"

char* get_abort_string(uint32_t code);

CO_SDO_abortCode_t read_SDO(CO_SDOclient_t* SDO_C, uint8_t nodeId, uint16_t index, uint8_t subIndex, uint8_t* buf, size_t bufSize, size_t* readSize);

CO_SDO_abortCode_t write_SDO(CO_SDOclient_t* SDO_C, uint8_t nodeId, uint16_t index, uint8_t subIndex, uint8_t* data, size_t dataSize);


CO_SDO_abortCode_t read_SDO_to_file(CO_SDOclient_t* SDO_C, uint8_t nodeId, uint16_t index, uint8_t subIndex, char *file_path);

CO_SDO_abortCode_t write_SDO_from_file(CO_SDOclient_t* SDO_C, uint8_t nodeId, uint16_t index, uint8_t subIndex, char *file_path);
#endif
