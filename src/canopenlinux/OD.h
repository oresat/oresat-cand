#ifndef OD_H
#define OD_H

#include <stdint.h>
#include "301/CO_ODinterface.h"

#define OD_CNT_NMT 1
#define OD_CNT_HB_PROD 1
#define OD_CNT_HB_CONS 0
#define OD_CNT_EM 1
#define OD_CNT_EM_PROD 1
#define OD_CNT_SDO_SRV 1
#define OD_CNT_SDO_CLI 1
#define OD_CNT_TIME 0
#define OD_CNT_SYNC 0
#define OD_CNT_RPDO 0
#define OD_CNT_TPDO 0

#define OD_CNT_ARR_1003 8

typedef struct {
    uint32_t x1000_device_type;
    uint8_t x1001_error_register;
    uint8_t x1003_predefined_error_field_sub0;
    uint32_t x1003_predefined_error_field[OD_CNT_ARR_1003];
    uint32_t x1014_cob_id_emergency_message;
    uint16_t x1017_producer_heartbeat_time;
    struct {
        uint8_t highest_index_supported;
        uint32_t vendor_id;
        uint32_t product_code;
        uint32_t revision_number;
        uint32_t serial_number;
    } x1018_identity;
    struct {
        uint8_t highest_index_supported;
        uint32_t cob_id_client_to_server;
        uint32_t cob_id_server_to_client;
        uint32_t node_id_od_sdo_client;
    } x1200_sdo_server_parameter;
    struct {
        uint8_t highest_index_supported;
        uint32_t cob_id_client_to_server;
        uint32_t cob_id_server_to_client;
        uint8_t node_id_of_sdo_server;
    } x1280_sdo_client_parameter;
} OD_RAM_t;

#ifndef OD_ATTR_RAM
#define OD_ATTR_RAM
#endif
extern OD_ATTR_RAM OD_RAM_t OD_RAM;

#ifndef OD_ATTR_OD
#define OD_ATTR_OD
#endif
extern OD_ATTR_OD OD_t *OD;

#define OD_ENTRY_H1000 &OD->list[0]
#define OD_ENTRY_H1001 &OD->list[1]
#define OD_ENTRY_H1003 &OD->list[2]
#define OD_ENTRY_H1014 &OD->list[3]
#define OD_ENTRY_H1017 &OD->list[4]
#define OD_ENTRY_H1018 &OD->list[5]
#define OD_ENTRY_H1200 &OD->list[6]
#define OD_ENTRY_H1280 &OD->list[7]

#define OD_ENTRY_H1000_DEVICE_TYPE &OD->list[0]
#define OD_ENTRY_H1001_ERROR_REGISTER &OD->list[1]
#define OD_ENTRY_H1003_PREDEFINED_ERROR_FIELD &OD->list[2]
#define OD_ENTRY_H1014_COB_ID_EMERGENCY_MESSAGE &OD->list[3]
#define OD_ENTRY_H1017_PRODUCER_HEARTBEAT_TIME &OD->list[4]
#define OD_ENTRY_H1018_IDENTITY &OD->list[5]
#define OD_ENTRY_H1200_SDO_SERVER_PARAMETER &OD->list[6]
#define OD_ENTRY_H1280_SDO_CLIENT_PARAMETER &OD->list[7]

#endif
