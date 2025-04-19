#include "CANopen.h"
#include "file_transfer_ext.h"
#include "parse_int.h"
#include "sdo_client.h"
#include "sdo_client_node.h"
#include <libgen.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern CO_t *CO;

static void usage(char *name) {
    printf("%s <interface> <node-id> <cache>\n", name);
    printf("\n");
    printf("caches: fread or fwrite\n");
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
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

    r = sdo_client_node_start(argv[1]);
    if (r < 0) {
        printf("node start failure: %d\n\n", -r);
        return EXIT_FAILURE;
    }

    int index;
    if (!strncmp(argv[3], "fread", strlen(argv[3]) + 1)) {
        index = FREAD_CACHE_INDEX;
    } else if (!strncmp(argv[3], "fwrite", strlen(argv[3]) + 1)) {
        index = FWRITE_CACHE_INDEX;
    } else {
        printf("invalid cache, must be fread or fwrite\n");
        return EXIT_FAILURE;
    }

    char *buf = NULL;
    CO_SDO_abortCode_t abort_code =
        sdo_read_str(CO->SDOclient, node_id, index, FILE_TRANSFER_SUBINDEX_FILES, &buf, false);
    if (abort_code != 0) {
        goto abort;
    } else if (!buf) {
        goto empty_buf;
    }

    if (strncmp(buf, "[]", strlen(buf))) {
        // could be done with cJSON
        char *token = strtok(buf, ",");
        while (token != NULL) {
            int len = strlen(token);
            if (token[len - 1] == ']') { // last file
                token[len - 2] = '\0';
            } else {
                token[len - 1] = '\0';
            }
            printf("%s\n", &token[2]);
            token = strtok(NULL, ",");
        }
    }

    free(buf);

empty_buf:
    sdo_client_node_stop();
    return EXIT_SUCCESS;

abort:
    sdo_client_node_stop();
    printf("SDO Abort: 0x%x - %s\n", abort_code, get_sdo_abort_string(abort_code));
    return EXIT_FAILURE;
}
