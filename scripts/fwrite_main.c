#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include "CANopen.h"
#include "system.h"
#include "file_transfer_ext.h"
#include "sdo_client_node.h"
#include "parse_int.h"
#include "sdo_client.h"

extern CO_t *CO;

static void usage(char *name) {
    printf("%s <interface> <node-id> <src> <optional-dest>\n", name);
}

int main(int argc, char* argv[]) {
    if ((argc != 4) && (argc != 5)) {
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

    char *file;
    if (argc == 4) {
        file = basename(argv[3]);
    } else {
        file = argv[4];
    }

    if (!is_file(argv[3])) {
        printf("file does not exist: %s\n", argv[3]);
        return EXIT_FAILURE;
    }

    r = sdo_client_node_start(argv[1]);
    if (r < 0) {
        printf("node start failure: %d\n", -r);
        return EXIT_FAILURE;
    }

    CO_SDO_abortCode_t abort_code = sdo_write_str(CO->SDOclient, node_id, FWRITE_CACHE_INDEX, FILE_TRANSFER_SUBINDEX_NAME, file);
    if (abort_code != 0) {
        goto abort;
    }

    abort_code = sdo_write_from_file(CO->SDOclient, node_id, FWRITE_CACHE_INDEX, FILE_TRANSFER_SUBINDEX_DATA, argv[3]);
    if (abort_code != 0) {
        goto abort;
    }

    sdo_client_node_stop();
    return EXIT_SUCCESS;

abort:
    sdo_client_node_stop();
    printf("SDO Abort: 0x%x - %s\n", abort_code, get_sdo_abort_string(abort_code));
    return EXIT_FAILURE;
}
