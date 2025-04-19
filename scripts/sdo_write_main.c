#include "CANopen.h"
#include "parse_int.h"
#include "sdo_client.h"
#include "sdo_client_node.h"
#include "str2buf.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern CO_t *CO;

static void usage(char *name) {
    printf("%s <interface> <node-id> <index> <subindex> <dtype> <value>\n", name);
    printf("\n");
    printf("dtypes: bool, int8, int16, int32, int64, uint8, uint16, "
           "uint32, uint64, float32, float64, string, bytes\n");
}

int main(int argc, char *argv[]) {
    if (argc != 7) {
        printf("invalid number of args\n\n");
        usage(argv[0]);
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

    r = sdo_client_node_start(argv[1]);
    if (r < 0) {
        return EXIT_FAILURE;
    }

    CO_SDO_abortCode_t abort_code = sdo_write(CO->SDOclient, node_id, index, subindex, data, data_size);
    if (abort_code != 0) {
        printf("SDO Abort: 0x%x - %s\n", abort_code, get_sdo_abort_string(abort_code));
    }

    sdo_client_node_stop();

    if (data != NULL) {
        free(data);
    }

    return EXIT_SUCCESS;
}
