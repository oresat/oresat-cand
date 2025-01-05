#ifndef CO_DRIVER_CUSTOM_H
#define CO_DRIVER_CUSTOM_H

#ifdef __cplusplus
extern "C" {
#endif

/* NMT configs */
#undef CO_CONFIG_NMT_MASTER
#define CO_CONFIG_NMT_MASTER 0

/* HB configs */
#undef CO_CONFIG_HB_ENABLE
#define CO_CONFIG_HB_ENABLE 0
#undef CO_CONFIG_HB_CONS
#define CO_CONFIG_HB_CONS 0

/* EM configs */
#undef CO_CONFIG_EM_CONSUMER
#define CO_CONFIG_EM_CONSUMER 0

/* TIME configs */
#undef CO_CONFIG_TIME
#define CO_CONFIG_TIME 0
#undef CO_CONFIG_TIME_ENABLE
#define CO_CONFIG_TIME_ENABLE 0

/* OD storage configs */
#undef CO_CONFIG_STORAGE_ENABLE
#define CO_CONFIG_STORAGE_ENABLE 0

/* SDO configs */
#undef CO_CONFIG_SDO_CLI
#define CO_CONFIG_SDO_CLI \
    (CO_CONFIG_SDO_CLI_ENABLE | CO_CONFIG_SDO_CLI_SEGMENTED |  CO_CONFIG_SDO_CLI_BLOCK)

/* SYNC configs */
#undef CO_CONFIG_SYNC
#define CO_CONFIG_SYNC \
    (CO_CONFIG_SYNC_ENABLE | CO_CONFIG_SYNC_PRODUCER)

/* LEDS configs */
#undef CO_CONFIG_LEDS_ENABLE
#define CO_CONFIG_LEDS_ENABLE 0

/* GFC configs */
#undef CO_CONFIG_GFC_ENABLE
#define CO_CONFIG_GFC_ENABLE 0

/* SRDO configs */
#undef CO_CONFIG_SRDO_ENABLE
#define CO_CONFIG_SRDO_ENABLE 0

/* LSS configs */
#undef CO_CONFIG_LSS_SLAVE
#define CO_CONFIG_LSS_SLAVE 0
#undef CO_CONFIG_LSS_MASTER
#define CO_CONFIG_LSS_MASTER 0

/* GTW configs */
#undef CO_CONFIG_GTW
#define CO_CONFIG_GTW 0

/* Trace configs */
#undef CO_CONFIG_TRACE_ENABLE
#define CO_CONFIG_TRACE_ENABLE 0

/* LSS config */
#undef CO_CONFIG_LSS
#define CO_CONFIG_LSS 0

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CO_DRIVER_CUSTOM_H */
