#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "CANopen.h"
#include "os_command_ext.h"
#include "sdo_client_node.h"
#include "parse_int.h"
#include "sdo_client.h"

extern CO_t *CO;

static void usage(char *name) {
    printf("%s <interface> <node-id> <command>\n", name);
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("invalid number of args\n");
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    int node_id;
    int r = parse_int_arg(argv[2], &node_id);
    if (r < 0) {
        printf("invalid node id: %s\n", argv[2]);
        return EXIT_FAILURE;
    }

    r = sdo_client_node_start(argv[1]);
    if (r < 0) {
        printf("node start failure: %d\n", -r);
        return EXIT_FAILURE;
    }

    CO_SDO_abortCode_t abort_code = sdo_write_str(CO->SDOclient, node_id, OS_CMD_INDEX, OS_CMD_SUBINDEX_COMMAND, argv[3]);
    if (abort_code != 0) {
        goto abort;
    }

    uint8_t status = OS_CMD_EXECUTING;
    while (status == OS_CMD_EXECUTING) {
        usleep(250000);
        abort_code = sdo_read_uint8(CO->SDOclient, node_id, OS_CMD_INDEX, OS_CMD_SUBINDEX_STATUS, &status);
        if (abort_code != 0) {
            goto abort;
        }
    }

    if ((status == OS_CMD_NO_ERROR_REPLY) || (status == OS_CMD_ERROR_REPLY)) {
        char *reply = NULL;
        abort_code = sdo_read_str(CO->SDOclient, node_id, OS_CMD_INDEX, OS_CMD_SUBINDEX_REPLY, &reply);
        if (abort_code != 0) {
            goto abort;
        }
        printf("%s", reply);
        free(reply);
    } else {
        printf("no reply\n");
    }

    sdo_client_node_stop();
    return EXIT_SUCCESS;

abort:
    sdo_client_node_stop();
    printf("SDO Abort: 0x%x - %s\n", abort_code, get_sdo_abort_string(abort_code));
    return EXIT_FAILURE;
}
