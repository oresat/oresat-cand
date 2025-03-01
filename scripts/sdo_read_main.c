#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "CANopen.h"
#include "sdo_client_node.h"
#include "parse_int.h"
#include "sdo_client.h"

extern CO_t *CO;

static void usage(char *name) {
    printf("%s <interface> <node-id> <index> <subindex> <dtype>\n", name);
    printf("\n");
    printf("dtypes: bool, int8, int16, int32, int64, uint8, uint16, "
           "uint32, uint64, float32, float64, string, bytes\n");
}

int main(int argc, char* argv[]) {
    if ((argc != 5) && (argc != 6)) {
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

    r = sdo_client_node_start(argv[1]);
    if (r < 0) {
        printf("node start failure: %d\n", -r);
        return EXIT_FAILURE;
    }

    void *data = NULL;
    size_t data_size = 0;
    CO_SDO_abortCode_t abort_code = sdo_read_dynamic(CO->SDOclient, node_id, index, subindex, &data, &data_size, false);
    sdo_client_node_stop();

    if (abort_code == 0) {
        if ((argc == 5) || (!strncmp(argv[5], "bytes", strlen(argv[5])))) {
            uint8_t *tmp = (uint8_t *)data;
            for (size_t i=0; i<data_size; i++) {
                printf("%02X", tmp[i]);
            }
            printf("\n");
        } else {
            char *dtype = argv[5];
            if (!strncmp(dtype, "bool", strlen(dtype))) {
                printf("%d\n", *(bool *)data);
            } else if (!strncmp(dtype, "int8", strlen(dtype))) {
                printf("%d (0x%X)\n", *(int8_t *)data, *(int8_t *)data);
            } else if (!strncmp(dtype, "int16", strlen(dtype))) {
                printf("%d (0x%X)\n", *(int16_t *)data, *(int16_t *)data);
            } else if (!strncmp(dtype, "int32", strlen(dtype))) {
                printf("%d (0x%X)\n", *(int32_t *)data, *(int32_t *)data);
            } else if (!strncmp(dtype, "int64", strlen(dtype))) {
                printf("%ld (0x%lX)\n", *(int64_t *)data, *(int64_t *)data);
            } else if (!strncmp(dtype, "uint8", strlen(dtype))) {
                printf("%d (0x%X)\n", *(uint8_t *)data, *(uint8_t *)data);
            } else if (!strncmp(dtype, "uint16", strlen(dtype))) {
                printf("%d (0x%X)\n", *(uint16_t *)data, *(uint16_t *)data);
            } else if (!strncmp(dtype, "uint32", strlen(dtype))) {
                printf("%d (0x%X)\n", *(uint32_t *)data, *(uint32_t *)data);
            } else if (!strncmp(dtype, "uint64", strlen(dtype))) {
                printf("%ld (0x%lX)\n", *(uint64_t *)data, *(uint64_t *)data);
            } else if (!strncmp(dtype, "float32", strlen(dtype))) {
                printf("%f\n", *(float *)data);
            } else if (!strncmp(dtype, "float64", strlen(dtype))) {
                printf("%f\n", *(double *)data);
            } else if (!strncmp(dtype, "string", strlen(dtype))) {
                printf("%s\n", (char *)data);
            }
        }
    } else {
        printf("SDO Abort: 0x%x - %s\n", abort_code, get_sdo_abort_string(abort_code));
    }

    return EXIT_SUCCESS;
}
