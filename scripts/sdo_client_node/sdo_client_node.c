#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sched.h>
#include <signal.h>
#include <stdarg.h>
#include <time.h>
#include <sys/epoll.h>
#include <sys/param.h>
#include <net/if.h>
#include <linux/reboot.h>
#include <sys/reboot.h>
#include <sys/syslog.h>

#include "CANopen.h"
#include "OD.h"
#include "CO_epoll_interface.h"

#include "log_prinf.h"
#include "sdo_client_node.h"

#define CO_GET_CO(obj)       ((uint16_t)(CO_##obj))
#define CO_GET_CNT(obj)      (uint8_t)(OD_CNT_##obj)
#define OD_GET(entry, index) OD_ENTRY_##entry

/* Interval of mainline and real-time thread in microseconds */
#ifndef MAIN_THREAD_INTERVAL_US
#define MAIN_THREAD_INTERVAL_US 100000
#endif

/* default values for CO_CANopenInit() */
#define NMT_CONTROL \
    (CO_NMT_STARTUP_TO_OPERATIONAL | CO_NMT_ERR_ON_ERR_REG | CO_ERR_REG_GENERIC_ERR | CO_ERR_REG_COMMUNICATION)
#define FIRST_HB_TIME 0
#define SDO_SRV_TIMEOUT_TIME 1000
#define SDO_CLI_TIMEOUT_TIME 500
#define SDO_CLI_BLOCK false
#define OD_STATUS_BITS NULL

CO_t* CO = NULL;
static int node_id = 0x7F;
static CO_epoll_t epMain;
static CO_CANptrSocketCan_t CANptr = {0};
static pthread_t thread_id;
static bool running = true;

static void* sdo_client_node_thread(void *arg);

static void sigHandler(int sig) {
    (void)sig;
    running = false;
}

int sdo_client_node_start(const char *CANdevice) {
    CO_ReturnError_t err;
    uint32_t errInfo = 0;

    log_printf_set_level(LOG_CRIT);

    CANptr.can_ifindex = if_nametoindex(CANdevice);
    if (CANptr.can_ifindex == 0) {
        log_printf(LOG_CRIT, DBG_NO_CAN_DEVICE, CANdevice);
        goto error;
    }

    uint32_t heapMemoryUsed = 0;
    CO = CO_new(NULL, &heapMemoryUsed);
    if (CO == NULL) {
        log_printf(LOG_CRIT, DBG_GENERAL, "CO_new(), heapMemoryUsed=", heapMemoryUsed);
        goto error;
    }

    if (signal(SIGINT, sigHandler) == SIG_ERR) {
        log_printf(LOG_CRIT, DBG_ERRNO, "signal(SIGINT, sigHandler)");
        goto error;
    }
    if (signal(SIGTERM, sigHandler) == SIG_ERR) {
        log_printf(LOG_CRIT, DBG_ERRNO, "signal(SIGTERM, sigHandler)");
        goto error;
    }

    err = CO_epoll_create(&epMain, MAIN_THREAD_INTERVAL_US);
    if (err != CO_ERROR_NO) {
        log_printf(LOG_CRIT, DBG_GENERAL, "CO_epoll_create(main), err=", err);
        goto error;
    }
    CANptr.epoll_fd = epMain.epoll_fd;

    CO_CANsetConfigurationMode((void*)&CANptr);
    CO_CANmodule_disable(CO->CANmodule);

    err = CO_CANinit(CO, (void*)&CANptr, 0);
    if (err != CO_ERROR_NO) {
        log_printf(LOG_CRIT, DBG_CAN_OPEN, "CO_CANinit()", err);
        goto error;
    }

#define CO_RX_IDX_SDO_CLI 2
#define CO_TX_IDX_SDO_CLI 2
    OD_entry_t* SDOcliPar = OD_GET(H1280, OD_H1280_SDO_CLIENT_1_PARAM);
    err = CO_SDOclient_init(&CO->SDOclient[0], OD, SDOcliPar, node_id, CO->CANmodule,
                            CO_RX_IDX_SDO_CLI, CO->CANmodule, CO_TX_IDX_SDO_CLI,
                            &errInfo);
    if (err != CO_ERROR_NO) {
        log_printf(LOG_CRIT, DBG_GENERAL, "CO_SDOclient_init(), err=", err);
        goto error;
    }

    CO_CANsetNormalMode(CO->CANmodule);

    log_printf(LOG_INFO, DBG_CAN_OPEN_INFO, node_id, "running ...");

    if (pthread_create(&thread_id, NULL, sdo_client_node_thread, NULL) != 0) {
        log_printf(LOG_CRIT, DBG_ERRNO, "pthread_create(rt_thread)");
    }
    return 0;

error:
    CO_epoll_close(&epMain);
    CO_CANsetConfigurationMode((void*)&CANptr);
    CO_delete(CO);
    return -1;
}

void sdo_client_node_stop(void) {
    running = false;
    if (pthread_join(thread_id, NULL) != 0) {
        log_printf(LOG_CRIT, DBG_ERRNO, "pthread_join()");
        exit(EXIT_FAILURE);
    }

    CO_epoll_close(&epMain);
    CO_CANsetConfigurationMode((void*)&CANptr);
    CO_delete(CO);

    log_printf(LOG_INFO, DBG_CAN_OPEN_INFO, node_id, "finished");
}

static void* sdo_client_node_thread(void *arg) {
    (void)arg;
    CO_NMT_reset_cmd_t reset = CO_RESET_NOT;
    running = true;
    while (running) {
        CO_epoll_wait(&epMain);
        CO_epoll_processRT(&epMain, CO, false);
        CO_epoll_processMain(&epMain, CO, false, &reset);
        CO_epoll_processLast(&epMain);
    }
    return NULL;
}
