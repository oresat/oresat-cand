#define OD_DEFINITION
#include "301/CO_ODinterface.h"
#include "OD.h"

#if CO_VERSION_MAJOR < 4
#error This file is only comatible with CANopenNode v4 and above
#endif

OD_ATTR_RAM OD_RAM_t OD_RAM = {
    .x1000_device_type = 0x0,
    .x1001_error_register = 0x0,
    .x1003_predefined_error_field_sub0 = 8,
    .x1003_predefined_error_field = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    .x1014_cob_id_emergency_message = 0x80,
    .x1017_producer_heartbeat_time = 0x3E8,
    .x1018_identity = {
        .highest_index_supported = 0x4,
        .vendor_id = 0x0,
        .product_code = 0x0,
        .revision_number = 0x0,
        .serial_number = 0x0,
    },
    .x1200_sdo_server_parameter = {
        .highest_index_supported = 0x3,
        .cob_id_client_to_server = 0x80000000,
        .cob_id_server_to_client = 0x80000000,
        .node_id_od_sdo_client = 0x1,
    },
    .x1280_sdo_client_parameter = {
        .highest_index_supported = 0x3,
        .cob_id_client_to_server = 0x80000000,
        .cob_id_server_to_client = 0x80000000,
        .node_id_of_sdo_server = 0x1,
    },
};

typedef struct {
    OD_obj_var_t o_1000_device_type;
    OD_obj_var_t o_1001_error_register;
    OD_obj_array_t o_1003_predefined_error_field;
    OD_obj_var_t o_1014_cob_id_emergency_message;
    OD_obj_var_t o_1017_producer_heartbeat_time;
    OD_obj_record_t o_1018_identity[5];
    OD_obj_record_t o_1200_sdo_server_parameter[4];
    OD_obj_record_t o_1280_sdo_client_parameter[4];
} ODObjs_t;

static CO_PROGMEM ODObjs_t ODObjs = {
    .o_1000_device_type = {
        .dataOrig = &OD_RAM.x1000_device_type,
        .attribute = ODA_SDO_R | ODA_TPDO | ODA_MB,
        .dataLength = 4
    },
    .o_1001_error_register = {
        .dataOrig = &OD_RAM.x1001_error_register,
        .attribute = ODA_SDO_R | ODA_TPDO,
        .dataLength = 1
    },
    .o_1003_predefined_error_field = {
        .dataOrig0 = &OD_RAM.x1003_predefined_error_field_sub0,
        .dataOrig = &OD_RAM.x1003_predefined_error_field[0],
        .attribute0 = ODA_SDO_R,
        .attribute = ODA_SDO_RW | ODA_TRPDO | ODA_MB,
        .dataElementLength = 4,
        .dataElementSizeof = sizeof(uint32_t),
    },
    .o_1014_cob_id_emergency_message = {
        .dataOrig = &OD_RAM.x1014_cob_id_emergency_message,
        .attribute = ODA_SDO_RW | ODA_TRPDO | ODA_MB,
        .dataLength = 4
    },
    .o_1017_producer_heartbeat_time = {
        .dataOrig = &OD_RAM.x1017_producer_heartbeat_time,
        .attribute = ODA_SDO_RW | ODA_TRPDO | ODA_MB,
        .dataLength = 2
    },
    .o_1018_identity = {
        {
            .dataOrig = &OD_RAM.x1018_identity.highest_index_supported,
            .subIndex = 0,
            .attribute = ODA_SDO_R,
            .dataLength = 1
        },
        {
            .dataOrig = &OD_RAM.x1018_identity.vendor_id,
            .subIndex = 1,
            .attribute = ODA_SDO_R | ODA_TPDO | ODA_MB,
            .dataLength = 4
        },
        {
            .dataOrig = &OD_RAM.x1018_identity.product_code,
            .subIndex = 2,
            .attribute = ODA_SDO_R | ODA_TPDO | ODA_MB,
            .dataLength = 4
        },
        {
            .dataOrig = &OD_RAM.x1018_identity.revision_number,
            .subIndex = 3,
            .attribute = ODA_SDO_R | ODA_TPDO | ODA_MB,
            .dataLength = 4
        },
        {
            .dataOrig = &OD_RAM.x1018_identity.serial_number,
            .subIndex = 4,
            .attribute = ODA_SDO_R | ODA_TPDO | ODA_MB,
            .dataLength = 4
        },
    },
    .o_1200_sdo_server_parameter = {
        {
            .dataOrig = &OD_RAM.x1200_sdo_server_parameter.highest_index_supported,
            .subIndex = 0,
            .attribute = ODA_SDO_R,
            .dataLength = 1
        },
        {
            .dataOrig = &OD_RAM.x1200_sdo_server_parameter.cob_id_client_to_server,
            .subIndex = 1,
            .attribute = ODA_SDO_RW | ODA_TRPDO | ODA_MB,
            .dataLength = 4
        },
        {
            .dataOrig = &OD_RAM.x1200_sdo_server_parameter.cob_id_server_to_client,
            .subIndex = 2,
            .attribute = ODA_SDO_RW | ODA_TRPDO | ODA_MB,
            .dataLength = 4
        },
        {
            .dataOrig = &OD_RAM.x1200_sdo_server_parameter.node_id_od_sdo_client,
            .subIndex = 3,
            .attribute = ODA_SDO_RW | ODA_TRPDO | ODA_MB,
            .dataLength = 4
        },
    },
    .o_1280_sdo_client_parameter = {
        {
            .dataOrig = &OD_RAM.x1280_sdo_client_parameter.highest_index_supported,
            .subIndex = 0,
            .attribute = ODA_SDO_R,
            .dataLength = 1
        },
        {
            .dataOrig = &OD_RAM.x1280_sdo_client_parameter.cob_id_client_to_server,
            .subIndex = 1,
            .attribute = ODA_SDO_RW | ODA_TRPDO | ODA_MB,
            .dataLength = 4
        },
        {
            .dataOrig = &OD_RAM.x1280_sdo_client_parameter.cob_id_server_to_client,
            .subIndex = 2,
            .attribute = ODA_SDO_RW | ODA_TRPDO | ODA_MB,
            .dataLength = 4
        },
        {
            .dataOrig = &OD_RAM.x1280_sdo_client_parameter.node_id_of_sdo_server,
            .subIndex = 3,
            .attribute = ODA_SDO_RW | ODA_TRPDO,
            .dataLength = 1
        },
    },
};

static OD_ATTR_OD OD_entry_t ODList[] = {
    {0x1000, 0x01, ODT_VAR, &ODObjs.o_1000_device_type, NULL},
    {0x1001, 0x01, ODT_VAR, &ODObjs.o_1001_error_register, NULL},
    {0x1003, 0x09, ODT_ARR, &ODObjs.o_1003_predefined_error_field, NULL},
    {0x1014, 0x01, ODT_VAR, &ODObjs.o_1014_cob_id_emergency_message, NULL},
    {0x1017, 0x01, ODT_VAR, &ODObjs.o_1017_producer_heartbeat_time, NULL},
    {0x1018, 0x05, ODT_REC, &ODObjs.o_1018_identity, NULL},
    {0x1200, 0x04, ODT_REC, &ODObjs.o_1200_sdo_server_parameter, NULL},
    {0x1280, 0x04, ODT_REC, &ODObjs.o_1280_sdo_client_parameter, NULL},
    {0x0000, 0x00, 0, NULL, NULL}
};

static OD_t _OD = {
    (sizeof(ODList) / sizeof(ODList[0])) - 1,
    &ODList[0]
};

OD_t *OD = &_OD;
