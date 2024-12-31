#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "CANopen.h"
#include "os_command_ext.h"
#include "basic_node.h"
#include "sdo_client.h"

extern CO_t *CO;

int parse_int_arg(char *arg, int *value) {
    char *endptr;
    int tmp;

    if ((arg[0] == '0') && (arg[1] == 'x')) { // hex
        tmp = strtoul(arg, &endptr, 16);
    } else {
        tmp = strtoul(arg, &endptr, 10);
    }

    if (endptr == arg) {
        return -1;
    }

    *value = tmp;
    return 0;
}

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

    size_t data_size = 1024;
    uint8_t data[data_size];
    size_t read_size;

    r = basic_node_start(argv[1]);
    if (r < 0) {
        printf("node start failure: %d\n", -r);
        return EXIT_FAILURE;
    }

    CO_SDO_abortCode_t abort_code = write_SDO(CO->SDOclient, node_id, OS_CMD_INDEX, OS_CMD_SUBINDEX_COMMAND, (void *)argv[3], strlen(argv[3]) + 1);
    if (abort_code != 0) {
        goto abort;
    }

    uint8_t status = OS_CMD_EXECUTING;
    while (status == OS_CMD_EXECUTING) {
        usleep(250000);
        abort_code = read_SDO(CO->SDOclient, node_id, OS_CMD_INDEX, OS_CMD_SUBINDEX_STATUS, &status, 1, &read_size);
        if (abort_code != 0) {
            goto abort;
        }
    }


    if ((status == OS_CMD_NO_ERROR_REPLY) || (status == OS_CMD_ERROR_REPLY)) {
        abort_code = read_SDO(CO->SDOclient, node_id, OS_CMD_INDEX, OS_CMD_SUBINDEX_REPLY, data, data_size, &read_size);
        if (abort_code != 0) {
            goto abort;
        }
        printf("%s", (char *)data);
    } else {
        printf("no reply\n");
    }

    basic_node_stop();
    return EXIT_SUCCESS;

abort:
    basic_node_stop();
    printf("SDO Abort: 0x%x - %s\n", abort_code, get_abort_string(abort_code));
    return EXIT_FAILURE;
}
