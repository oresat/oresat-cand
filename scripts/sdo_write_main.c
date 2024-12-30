#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "CANopen.h"
#include "basic_node.h"
#include "str2buf.h"
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

static void usage(void) {
    printf("./sdo-write <interface> <node-id> <index> <subindex> <dtype> <write-arg>\n");
}

int main(int argc, char* argv[]) {
    if (argc != 7) {
        printf("invalid number of args\n");
        usage();
        return EXIT_FAILURE;
    }

    int node_id;
    int r = parse_int_arg(argv[2], &node_id);
    if (r < 0) {
        printf("invalid node id: %s\n", argv[2]);
        return EXIT_FAILURE;
    }

    int index;
    r = parse_int_arg(argv[3], &index);
    if (r < 0) {
        printf("invalid index: %s\n", argv[3]);
        return EXIT_FAILURE;
    }

    int subindex;
    r = parse_int_arg(argv[4], &subindex);
    if (r < 0) {
        printf("invalid subindex: %s\n", argv[4]);
        return EXIT_FAILURE;
    }

    size_t data_size;
    uint8_t *data = str2buf_dtype_str(argv[5], argv[6], &data_size);
    if (data == NULL) {
        printf("invalid dtype or value: %s or %s\n", argv[5], argv[6]);
        return EXIT_FAILURE;
    }

    r = basic_node_start(argv[1]);
    if (r < 0) {
        return EXIT_FAILURE;
    }

    CO_SDO_abortCode_t abort_code = write_SDO(CO->SDOclient, node_id, index, subindex, data, data_size);
    if (abort_code != 0) {
        printf("SDO Abort: 0x%x - %s\n", abort_code, get_abort_string(abort_code));
    }

    basic_node_stop();

    if (data != NULL) {
        free(data);
    }

    return EXIT_SUCCESS;
}
