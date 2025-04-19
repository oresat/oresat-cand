#ifndef _CO_DRIVER_CUSTOM_H_
#define _CO_DRIVER_CUSTOM_H_

#define CO_CONFIG_TIME    0
#define CO_CONFIG_GTW     0
#define CO_CONFIG_LSS     0
#define CO_CONFIG_LEDS    0
#define CO_CONFIG_STORAGE 0

// hacky way to disable GFC and SRDO
#undef CO_CONFIG_GFC_ENABLE
#define CO_CONFIG_GFC_ENABLE 0
#undef CO_CONFIG_SRDO_ENABLE
#define CO_CONFIG_SRDO_ENABLE 0

#endif
