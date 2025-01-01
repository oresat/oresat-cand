#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "CANopen.h"
#include "system.h"
#include "file_transfer_ext.h"
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
    printf("%s <interface> <node-id> <cache>\n", name);
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

    r = basic_node_start(argv[1]);
    if (r < 0) {
        printf("node start failure: %d\n", -r);
        return EXIT_FAILURE;
    }

    int index;
    size_t buf_size = 1024;
    char buf[buf_size];
    size_t read_size = 0;

    if (!strncmp(argv[3], "fread", strlen(argv[3])+1)) {
        index = FREAD_CACHE_INDEX;
    } else if (!strncmp(argv[3], "fwrite", strlen(argv[3])+1)) {
        index = FWRITE_CACHE_INDEX;
    } else {
        printf("invalid cache, must be fread or fwrite\n");
        return EXIT_FAILURE;
    }

    CO_SDO_abortCode_t abort_code = read_SDO(CO->SDOclient, node_id, index, FILE_TRANSFER_SUBINDEX_FILES, (void *)buf, buf_size, &read_size);
    if (abort_code != 0) {
        goto abort;
    }

    buf[read_size] = '\0';

    // could be done with cJSON
    if (strncmp(buf, "[]", 3)) {
        char *token = strtok(buf, ",");
        while (token != NULL) {
            int len = strlen(token);
            if (token[0] == '[') { // first file
                token[len-1] = '\0';
                printf("%s\n", &token[2]);
            } else if (token[strlen(token)-1] == ']') { // last file
                token[len-2] = '\0';
                printf("%s\n", &token[2]);
            } else {
                token[len-1] = '\0';
                printf("%s\n", &token[2]);
            }
            token = strtok(NULL, ",");
        }
    }

    basic_node_stop();
    return EXIT_SUCCESS;

abort:
    basic_node_stop();
    printf("SDO Abort: 0x%x - %s\n", abort_code, get_abort_string(abort_code));
    return EXIT_FAILURE;
}
