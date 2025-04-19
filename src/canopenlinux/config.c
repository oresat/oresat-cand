#define OD_DEFINITION
#include "301/CO_ODinterface.h"
#include "CANopen.h"

int fill_config(OD_t *od, CO_config_t *config) {
    if (!od || !config) {
        return -1;
    }

    // nmt
    config->ENTRY_H1017 = OD_find(od, 0x1017);
    config->CNT_NMT = config->ENTRY_H1017 != NULL;
    // hb
    config->ENTRY_H1016 = OD_find(od, 0x1016);
    config->CNT_HB_CONS = config->ENTRY_H1016 != NULL;
    config->CNT_ARR_1016 =
        config->ENTRY_H1016 ? ((OD_obj_array_t *)config->ENTRY_H1016->odObject)->dataElementLength : 0;
    // emcy
    config->ENTRY_H1003 = OD_find(od, 0x1003);
    config->CNT_EM = config->ENTRY_H1003 != NULL;
    config->CNT_ARR_1003 =
        config->ENTRY_H1003 ? ((OD_obj_array_t *)config->ENTRY_H1003->odObject)->dataElementLength : 0;
    config->ENTRY_H1001 = OD_find(od, 0x1001);
    config->ENTRY_H1014 = OD_find(od, 0x1014);
    config->ENTRY_H1015 = OD_find(od, 0x1015);
    // sdo server
    config->ENTRY_H1200 = OD_find(od, 0x1200);
    config->CNT_SDO_SRV = config->ENTRY_H1200 != NULL;
    // sdo client
    config->ENTRY_H1280 = OD_find(od, 0x1280);
    config->CNT_SDO_CLI = config->ENTRY_H1280 != NULL;
    // time
    config->ENTRY_H1012 = OD_find(od, 0x1012);
    config->CNT_TIME = config->ENTRY_H1012 != NULL;
    // sync
    config->ENTRY_H1005 = OD_find(od, 0x1005);
    config->CNT_SYNC = config->ENTRY_H1005 != NULL;
    config->ENTRY_H1006 = OD_find(od, 0x1006);
    config->ENTRY_H1007 = OD_find(od, 0x1007);
    config->ENTRY_H1019 = OD_find(od, 0x1019);
    // rpdo
    config->CNT_RPDO = 0;
    config->ENTRY_H1400 = OD_find(od, 0x1400);
    OD_entry_t *entry = config->ENTRY_H1400;
    while (entry && (entry->index < 0x1600)) {
        config->CNT_RPDO++;
        entry++;
    }
    config->ENTRY_H1600 = OD_find(od, 0x1600);
    // tpdo
    config->CNT_TPDO = 0;
    config->ENTRY_H1800 = OD_find(od, 0x1800);
    entry = config->ENTRY_H1800;
    while (entry && (entry->index < 0x1A00)) {
        config->CNT_TPDO++;
        entry++;
    }
    config->ENTRY_H1A00 = OD_find(od, 0x1A00);
    // other non-CiA301
    config->CNT_LEDS = 0;
    config->CNT_GFC = 0;
    config->ENTRY_H1300 = NULL;
    config->CNT_SRDO = 0;
    config->ENTRY_H1301 = NULL;
    config->ENTRY_H1381 = NULL;
    config->ENTRY_H13FE = NULL;
    config->ENTRY_H13FF = NULL;
    config->CNT_LSS_SLV = 0;
    config->CNT_LSS_MST = 0;
    config->CNT_GTWA = 0;
    config->CNT_TRACE = 0;

    return 0;
}
