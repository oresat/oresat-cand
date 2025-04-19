#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sched.h>
#include <signal.h>
#include <stdarg.h>
#include <syslog.h>
#include <time.h>
#include <sys/epoll.h>
#include <net/if.h>
#include <linux/reboot.h>
#include <sys/reboot.h>
#include <wordexp.h>
#include "logger.h"
#include "CANopen.h"
#include "OD.h"
#include "config.h"
#include "CO_epoll_interface.h"
#include "fcache.h"
#include "ecss_time_ext.h"
#include "os_command_ext.h"
#include "file_transfer_ext.h"
#include "system_ext.h"
#include "conf_load.h"
#include "system.h"
#include "ipc.h"

#define MAIN_THREAD_INTERVAL_US 100000
#define TMR_THREAD_INTERVAL_US 1000

#define NMT_CONTROL \
    (CO_NMT_STARTUP_TO_OPERATIONAL | CO_NMT_ERR_ON_ERR_REG | CO_ERR_REG_GENERIC_ERR | CO_ERR_REG_COMMUNICATION)
#define FIRST_HB_TIME 500
#define SDO_SRV_TIMEOUT_TIME 1000
#define SDO_CLI_TIMEOUT_TIME 500
#define SDO_CLI_BLOCK false

static CO_t* CO = NULL;
static OD_t *od = NULL;
static CO_config_t config;
static CO_config_t base_config;
static fcache_t* fread_cache = NULL;
static fcache_t* fwrite_cache = NULL;

static uint8_t CO_activeNodeId = 0x7C;
static CO_epoll_t epRT;
static void* rt_thread(void* arg);
static void* ipc_responder_thread(void* arg);
static void* ipc_consumer_thread(void* arg);
static void* ipc_monitor_thread(void* arg);
static volatile sig_atomic_t CO_endProgram = 0;

static void
sigHandler(int sig) {
    (void)sig;
    CO_endProgram = 1;
}

static void
EmergencyRxCallback(const uint16_t ident, const uint16_t errorCode, const uint8_t errorRegister, const uint8_t errorBit, const uint32_t infoCode) {
    int16_t nodeIdRx = ident ? (ident & 0x7F) : CO_activeNodeId;

    log_printf(LOG_NOTICE, DBG_EMERGENCY_RX, nodeIdRx, errorCode, errorRegister, errorBit, infoCode);
    ipc_broadcast_emcy(nodeIdRx, errorCode, infoCode);
}

static char*
NmtState2Str(CO_NMT_internalState_t state) {
    switch (state) {
        case CO_NMT_INITIALIZING: return "initializing";
        case CO_NMT_PRE_OPERATIONAL: return "pre-operational";
        case CO_NMT_OPERATIONAL: return "operational";
        case CO_NMT_STOPPED: return "stopped";
        default: return "unknown";
    }
}

static void
NmtChangedCallback(CO_NMT_internalState_t state) {
    log_printf(LOG_NOTICE, DBG_NMT_CHANGE, NmtState2Str(state), state);
}

static void
HeartbeatNmtChangedCallback(uint8_t nodeId, uint8_t idx, CO_NMT_internalState_t state, void* object) {
    (void)object;
    log_printf(LOG_NOTICE, DBG_HB_CONS_NMT_CHANGE, nodeId, idx, NmtState2Str(state), state);
    ipc_broadcast_hb(nodeId, state);
}

static void
printUsage(char* progName) {
    printf("Usage: %s <CAN interface> [options]\n", progName);
    printf("\n");
    printf("Options:\n");
    printf("  -m                  Network manager node\n");
    printf("  -n                  Set node id\n");
    printf("  -o                  Load od config\n");
    printf("  -p <RT priority>    Real-time priority of RT thread (1 .. 99). If not set or\n");
    printf("                      set to -1, then normal scheduler is used for RT thread.\n");
    printf("  -v                  Verbose logging\n");
}

static void fix_cob_ids(OD_t *od, uint8_t node_id)
{
    if (!od) {
        return;
    }
    int i;
    uint32_t cob_id;
	  OD_entry_t *entry;
    for (int e=0; e < od->size; e++) {
        entry = &od->list[e];
        if ((entry->index >= 0x1400) && (entry->index < 0x1600)) {
            i = entry->index - 0x1400;
            OD_get_u32(entry, 1, &cob_id, true);
            if ((cob_id & 0x7FF) == (0x200U + (0x100U * (i % 4) + (i / 4)))) {
                OD_set_u32(entry, 1, cob_id + node_id, true);
            }
        }
        if ((entry->index >= 0x1800) && (entry->index < 0x1A00)) {
            i = entry->index - 0x1800;
            OD_get_u32(entry, 1, &cob_id, true);
            if ((cob_id & 0x7FF) == (0x180U + (0x100U * (i % 4) + (i / 4)))) {
                OD_set_u32(entry, 1, cob_id + node_id, true);
            }
        }
    }
}

int
main(int argc, char* argv[]) {
    int programExit = EXIT_SUCCESS;
    CO_epoll_t epMain;
    pthread_t rt_thread_id;
    pthread_t ipc_responder_thread_id;
    pthread_t ipc_consumer_thread_id;
    pthread_t ipc_monitor_thread_id;
    int rtPriority = -1;
    CO_NMT_reset_cmd_t reset = CO_RESET_NOT;
    CO_ReturnError_t err;
    CO_CANptrSocketCan_t CANptr = {0};
    int opt;
    bool firstRun = true;
    char* CANdevice = NULL;
    char dcf_path[PATH_MAX] = {0};
    bool used_extenal_od = false;
    bool network_manager_node = false;

    if (argc < 2) {
        printUsage(argv[0]);
        exit(EXIT_FAILURE);
    }
    while ((opt = getopt(argc, argv, "hmn:o:p:v")) != -1) {
        switch (opt) {
            case 'h':
                printUsage(argv[0]);
                exit(EXIT_SUCCESS);
            case 'm':
                network_manager_node = true;
                break;
            case 'n':
                CO_activeNodeId = strtol(optarg, NULL, 16);
                break;
            case 'o':
                strncpy(dcf_path, optarg, strlen(optarg));
                break;
            case 'p':
                rtPriority = strtol(optarg, NULL, 0);
                break;
            case 'v':
                log_level_set(LOG_DEBUG);
                break;
            default:
                printUsage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (optind < argc) {
        CANdevice = argv[optind];
        CANptr.can_ifindex = if_nametoindex(CANdevice);
    }
    if (CANdevice == NULL) {
        log_critical("no CAN interface arg");
        exit(EXIT_FAILURE);
    }

    if (rtPriority != -1
        && (rtPriority < sched_get_priority_min(SCHED_FIFO) || rtPriority > sched_get_priority_max(SCHED_FIFO))) {
        log_printf(LOG_CRIT, DBG_WRONG_PRIORITY, rtPriority);
        printUsage(argv[0]);
        exit(EXIT_FAILURE);
    }

    log_info("starting %s v%s", PROJECT_NAME, PROJECT_VERSION);

    bool first_interface_check = true;
    do {
        CANptr.can_ifindex = if_nametoindex(CANdevice);
        if ((first_interface_check) && (CANptr.can_ifindex == 0)) {
            log_critical("can't find CAN interface %s", CANdevice);
            first_interface_check = false;
        }
        sleep_ms(250);
    } while (CANptr.can_ifindex == 0);
    if (!first_interface_check) {
        log_info("found CAN interface %s", CANdevice);
    }

    if (dcf_path[0] != '\0') {
        if (od_conf_load(dcf_path, &od, !network_manager_node) < 0) {
            log_critical("failed to load in external od from %s", dcf_path);
        } else {
            log_info("using external od from %s", dcf_path);
            used_extenal_od = true;
        }
    }
    if (od == NULL) {
        log_info("using internal od", dcf_path);
        od = OD;
        used_extenal_od = false;
    }
    fix_cob_ids(od, CO_activeNodeId);
    fill_config(od, &config);
    fill_config(OD, &base_config);

    uint32_t heapMemoryUsed = 0;
    CO = CO_new(&config, &heapMemoryUsed);
    if (CO == NULL) {
        log_printf(LOG_CRIT, DBG_GENERAL, "CO_new(), heapMemoryUsed=", heapMemoryUsed);
        exit(EXIT_FAILURE);
    }

    if (signal(SIGINT, sigHandler) == SIG_ERR) {
        log_printf(LOG_CRIT, DBG_ERRNO, "signal(SIGINT, sigHandler)");
        exit(EXIT_FAILURE);
    }
    if (signal(SIGTERM, sigHandler) == SIG_ERR) {
        log_printf(LOG_CRIT, DBG_ERRNO, "signal(SIGTERM, sigHandler)");
        exit(EXIT_FAILURE);
    }

    err = CO_epoll_create(&epMain, MAIN_THREAD_INTERVAL_US);
    if (err != CO_ERROR_NO) {
        log_printf(LOG_CRIT, DBG_GENERAL, "CO_epoll_create(main), err=", err);
        exit(EXIT_FAILURE);
    }
    err = CO_epoll_create(&epRT, TMR_THREAD_INTERVAL_US);
    if (err != CO_ERROR_NO) {
        log_printf(LOG_CRIT, DBG_GENERAL, "CO_epoll_create(RT), err=", err);
        exit(EXIT_FAILURE);
    }
    CANptr.epoll_fd = epRT.epoll_fd;

    if (network_manager_node == false) {
        if (getuid() == 0)  {
            fread_cache = fcache_init("/var/cache/oresat/fread");
            fwrite_cache = fcache_init("/var/cache/oresat/fwrite");
        } else {
            log_warning("not running as root");
            char tmp_path[PATH_MAX];
            wordexp_t exp_result;
            wordexp("~/.cache/oresat", &exp_result, 0);
            sprintf(tmp_path, "%s/%s", exp_result.we_wordv[0], "fread");
            fread_cache = fcache_init(tmp_path);
            sprintf(tmp_path, "%s/%s", exp_result.we_wordv[0], "fwrite");
            fwrite_cache = fcache_init(tmp_path);
            wordfree(&exp_result);
        }
        log_info("fread cache path: %s", fread_cache->dir_path);
        log_info("fwrite cache path: %s", fwrite_cache->dir_path);

        os_command_extension_init(od);
        ecss_time_extension_init(od);
        file_transfer_extension_init(od, fread_cache, fwrite_cache);
        system_extension_init(od);
    }

    ipc_init(od);



    while ((reset != CO_RESET_APP) && (reset != CO_RESET_QUIT) && (CO_endProgram == 0)) {
        uint32_t errInfo;

        if (!firstRun) {
            CO_LOCK_OD(CO->CANmodule);
            CO->CANmodule->CANnormal = false;
            CO_UNLOCK_OD(CO->CANmodule);
        }

        CO_CANsetConfigurationMode((void*)&CANptr);
        CO_CANmodule_disable(CO->CANmodule);

        err = CO_CANinit(CO, (void*)&CANptr, 0 /* bit rate not used */);
        if (err != CO_ERROR_NO) {
            log_printf(LOG_CRIT, DBG_CAN_OPEN, "CO_CANinit()", err);
            programExit = EXIT_FAILURE;
            CO_endProgram = 1;
            continue;
        }

        errInfo = 0;
        err = CO_CANopenInit(CO, NULL, NULL, od, NULL, NMT_CONTROL, FIRST_HB_TIME,
                             SDO_SRV_TIMEOUT_TIME, SDO_CLI_TIMEOUT_TIME,
                             SDO_CLI_BLOCK, CO_activeNodeId, &errInfo);
        if (err != CO_ERROR_NO) {
            if (err == CO_ERROR_OD_PARAMETERS) {
                log_printf(LOG_CRIT, DBG_OD_ENTRY, errInfo);
            } else {
                log_printf(LOG_CRIT, DBG_CAN_OPEN, "CO_CANopenInit()", err);
            }
            programExit = EXIT_FAILURE;
            CO_endProgram = 1;
            continue;
        }

        CO_epoll_initCANopenMain(&epMain, CO);
        if (!CO->nodeIdUnconfigured) {
            if (errInfo != 0) {
                CO_errorReport(CO->em, CO_EM_INCONSISTENT_OBJECT_DICT, CO_EMC_DATA_SET, errInfo);
            }
            if (config.CNT_EM) {
                CO_EM_initCallbackRx(CO->em, EmergencyRxCallback);
            }
            CO_NMT_initCallbackChanged(CO->NMT, NmtChangedCallback);
            if (config.CNT_HB_CONS) {
                CO_HBconsumer_initCallbackNmtChanged(CO->HBcons, 0, NULL, HeartbeatNmtChangedCallback);
            }

            log_printf(LOG_INFO, DBG_CAN_OPEN_INFO, CO_activeNodeId, "communication reset");
        } else {
            log_printf(LOG_INFO, DBG_CAN_OPEN_INFO, CO_activeNodeId, "node-id not initialized");
        }

        if (firstRun) {
            firstRun = false;

            if (pthread_create(&rt_thread_id, NULL, rt_thread, NULL) != 0) {
                log_printf(LOG_CRIT, DBG_ERRNO, "pthread_create(rt_thread)");
                programExit = EXIT_FAILURE;
                CO_endProgram = 1;
                continue;
            }
            if (rtPriority > 0) {
                struct sched_param param;

                param.sched_priority = rtPriority;
                if (pthread_setschedparam(rt_thread_id, SCHED_FIFO, &param) != 0) {
                    log_printf(LOG_CRIT, DBG_ERRNO, "pthread_setschedparam()");
                    programExit = EXIT_FAILURE;
                    CO_endProgram = 1;
                    continue;
                }
            }
            if (pthread_create(&ipc_responder_thread_id, NULL, ipc_responder_thread, NULL) != 0) {
                log_printf(LOG_CRIT, DBG_ERRNO, "pthread_create(ipc_responder_thread)");
                programExit = EXIT_FAILURE;
                CO_endProgram = 1;
                continue;
            }
            if (pthread_create(&ipc_consumer_thread_id, NULL, ipc_consumer_thread, NULL) != 0) {
                log_printf(LOG_CRIT, DBG_ERRNO, "pthread_create(ipc_consumer_thread)");
                programExit = EXIT_FAILURE;
                CO_endProgram = 1;
                continue;
            }
            if (pthread_create(&ipc_monitor_thread_id, NULL, ipc_monitor_thread, NULL) != 0) {
                log_printf(LOG_CRIT, DBG_ERRNO, "pthread_create(ipc_monitor_thread)");
                programExit = EXIT_FAILURE;
                CO_endProgram = 1;
                continue;
            }
        }

        errInfo = 0;
        err = CO_CANopenInitPDO(CO, CO->em, od, CO_activeNodeId, &errInfo);
        if (err != CO_ERROR_NO) {
            if (err == CO_ERROR_OD_PARAMETERS) {
                log_printf(LOG_CRIT, DBG_OD_ENTRY, errInfo);
            } else {
                log_printf(LOG_CRIT, DBG_CAN_OPEN, "CO_CANopenInitPDO()", err);
            }
            programExit = EXIT_FAILURE;
            CO_endProgram = 1;
            continue;
        }

        CO_CANsetNormalMode(CO->CANmodule);

        log_printf(LOG_INFO, DBG_CAN_OPEN_INFO, CO_activeNodeId, "running ...");

        reset = CO_RESET_NOT;
        while (reset == CO_RESET_NOT && CO_endProgram == 0) {
            CO_epoll_wait(&epMain);
            CO_epoll_processMain(&epMain, CO, false, &reset);
            CO_epoll_processLast(&epMain);

            static uint32_t last_check = 0;
            uint32_t uptime_s = get_uptime_s();
            if (uptime_s != last_check) {
                ipc_broadcast_bus_status(CO);
                last_check = uptime_s;
            }
        }
    }

    CO_endProgram = 1;

    ipc_free();

    if (pthread_join(rt_thread_id, NULL) != 0) {
        log_printf(LOG_CRIT, DBG_ERRNO, "pthread_join()");
        exit(EXIT_FAILURE);
    }
    if (pthread_join(ipc_responder_thread_id, NULL) != 0) {
        log_printf(LOG_CRIT, DBG_ERRNO, "pthread_join()");
        exit(EXIT_FAILURE);
    }
    if (pthread_join(ipc_consumer_thread_id, NULL) != 0) {
        log_printf(LOG_CRIT, DBG_ERRNO, "pthread_join()");
        exit(EXIT_FAILURE);
    }
    if (pthread_join(ipc_monitor_thread_id, NULL) != 0) {
        log_printf(LOG_CRIT, DBG_ERRNO, "pthread_join()");
        exit(EXIT_FAILURE);
    }

    if (network_manager_node == false) {
        os_command_extension_free();
        file_transfer_extension_free();

        fcache_free(fread_cache);
        fcache_free(fwrite_cache);
    }

    CO_epoll_close(&epRT);
    CO_epoll_close(&epMain);
    CO_CANsetConfigurationMode((void*)&CANptr);
    CO_delete(CO);

    if (used_extenal_od && (od != NULL)) {
        od_conf_free(od, !network_manager_node);
    }

    log_printf(LOG_INFO, DBG_CAN_OPEN_INFO, CO_activeNodeId, "finished");
    exit(programExit);
}

static void*
rt_thread(void* arg) {
    (void)arg;
    while (CO_endProgram == 0) {
        CO_epoll_wait(&epRT);
        CO_epoll_processRT(&epRT, CO, true);
        CO_epoll_processLast(&epRT);
    }
    return NULL;
}

static void*
ipc_responder_thread(void* arg) {
    (void)arg;
    while (CO_endProgram == 0) {
        ipc_responder_process(CO, od, &config, fread_cache);
    }
    return NULL;
}

static void*
ipc_consumer_thread(void* arg) {
    (void)arg;
    while (CO_endProgram == 0) {
        ipc_consumer_process(CO, od, &base_config, &config);
    }
    return NULL;
}

static void*
ipc_monitor_thread(void* arg) {
    (void)arg;
    while (CO_endProgram == 0) {
        ipc_monitor_process();
    }
    return NULL;
}
