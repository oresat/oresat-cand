#include "ipc_responder.h"
#include "CANopen.h"
#include "ipc_msg.h"
#include "logger.h"
#include <CANopen.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <zmq.h>

#define ZMQ_HEADER_LEN 5

#define LOG_RESPONDER "responder: "

static void *responder = NULL;

static int responder_sdo_read(uint8_t *buffer_in, int buffer_in_recv, uint8_t *buffer_out, int buffer_out_max_len,
                              CO_t *co);
static int responder_add_file(uint8_t *buffer_in, int buffer_in_recv, uint8_t *buffer_out, int buffer_out_max_len,
                              fcache_t *fread_cache);
static int responder_sdo_read_file(uint8_t *buffer_in, int buffer_in_recv, uint8_t *buffer_out, int buffer_out_max_len,
                                   CO_t *co);
static int responder_sdo_write_file(uint8_t *buffer_in, int buffer_in_recv, uint8_t *buffer_out, int buffer_out_max_len,
                                    CO_t *co);
static int responder_sdo_files_list(uint8_t *buffer_in, int buffer_in_recv, uint8_t *buffer_out, int buffer_out_max_len,
                                    CO_t *co);

int ipc_responder_init(void *context) {
    if (!context) {
        return -EINVAL;
    }
    responder = zmq_socket(context, ZMQ_ROUTER);
    zmq_bind(responder, "tcp://*:6000");

    return 0;
}

void ipc_responder_process(CO_t *co, OD_t *od, CO_config_t *config, fcache_t *fread_cache) {
    if (!co || !od || !config) {
        log_error(LOG_RESPONDER "ipc process null arg");
        return;
    }

    static uint8_t buffer_in_full[IPC_BUFFER_SIZE];
    uint8_t *buffer_in = &buffer_in_full[1];

    static uint8_t buffer_out_full[IPC_BUFFER_SIZE];
    buffer_out_full[0] = IPC_MSG_VERSION;
    uint8_t *buffer_out = &buffer_out_full[1];

    uint8_t header[ZMQ_HEADER_LEN];

    zmq_msg_t msg;
    int r = zmq_msg_init(&msg);
    if (r != 0) {
        return;
    }

    int nbytes = 0;
    nbytes = zmq_msg_recv(&msg, responder, 0);
    if (nbytes < 0) {
        zmq_msg_close(&msg);
        return;
    }
    if (nbytes != ZMQ_HEADER_LEN) {
        log_error(LOG_RESPONDER "zmq msg recv header error %d", errno);
        zmq_msg_close(&msg);
        return;
    } else if (nbytes != ZMQ_HEADER_LEN) {
        log_error(LOG_RESPONDER "unexpected header len %d", nbytes);
        zmq_msg_close(&msg);
        return;
    }
    memcpy(header, zmq_msg_data(&msg), nbytes);

    nbytes = zmq_msg_recv(&msg, responder, 0);
    if (nbytes != 0) {
        log_error(LOG_RESPONDER "zmq msg recv header error");
        zmq_msg_close(&msg);
        return;
    }

    nbytes = zmq_msg_recv(&msg, responder, 0);
    if (nbytes == -1) {
        log_error(LOG_RESPONDER "zmq msg recv error %d", errno);
        zmq_msg_close(&msg);
        return;
    }

    if (nbytes <= 2) {
        log_error(LOG_RESPONDER "ipc msg is to small at %d bytes", nbytes);
        zmq_msg_close(&msg);
        return;
    }

    memcpy(&buffer_in, zmq_msg_data(&msg), nbytes);
    int buffer_in_recv = nbytes - 1; // minus 1 for IPC_MSG_VERSION

    zmq_msg_close(&msg);

    if (buffer_in_full[0] != IPC_MSG_VERSION) {
        log_error(LOG_RESPONDER "expected ipc protocal version %d not %d", IPC_MSG_VERSION, buffer_in_full[0]);
        return;
    }

    int buffer_out_send = 0;

    // default response
    buffer_out[0] = IPC_MSG_ID_ERROR;
    buffer_out_send = 1;

    switch (buffer_in[0]) {
    default: {
        log_debug(LOG_RESPONDER "unknown msg id %d", buffer_in[0]);
        buffer_out[0] = IPC_MSG_ID_ERROR_UNKNOWN_ID;
        buffer_out_send = 1;
        break;
    }
    }

    zmq_send(responder, header, 5, ZMQ_SNDMORE);
    zmq_send(responder, header, 0, ZMQ_SNDMORE);
    zmq_send(responder, buffer_out_full, buffer_out_send + 1, 0);
}

void ipc_responder_free(void) {
    zmq_close(responder);
}

static int responder_sdo_read(uint8_t *buffer_in, int buffer_in_recv, uint8_t *buffer_out, int buffer_out_max_len,
                              CO_t *co) {
    int r = -EINVAL;
    if (!buffer_in || buffer_in_recv < 1 || !buffer_out) {
        if (buffer_in_recv == sizeof(ipc_msg_sdo_t)) {
            ipc_msg_sdo_t msg_sdo;
            memcpy(&msg_sdo, buffer_in, sizeof(ipc_msg_sdo_t));
            log_debug(LOG_RESPONDER "sdo read node 0x%X index 0x%X subindex 0x%X", msg_sdo.node_id, msg_sdo.index,
                      msg_sdo.subindex);
            void *data = NULL;
            size_t data_len = 0;
            CO_SDO_abortCode_t ac = -1;
            if (co->SDOclient) {
                ac = sdo_read_dynamic(co->SDOclient, msg_sdo.node_id, msg_sdo.index, msg_sdo.subindex, &data, &data_len,
                                      false);
            }
            int buffer_out_send = 0;
            if (ac == 0) {
                if (data == NULL) {
                    memcpy(buffer_out, buffer_in, buffer_in_recv);
                    buffer_out_send = buffer_in_recv;
                } else if ((buffer_in_recv + data_len) > buffer_out_max_len) {
                    free(data);
                    ipc_msg_error_abort_t msg_error_abort = {
                        .id = IPC_MSG_ID_ERROR_ABORT,
                        .abort_code = CO_SDO_AB_DATA_LONG,
                    };
                    buffer_out_send = sizeof(ipc_msg_error_abort_t);
                    memcpy(buffer_out, &msg_error_abort, buffer_out_send);
                } else {
                    memcpy(buffer_out, buffer_in, buffer_in_recv);
                    memcpy(&buffer_out[buffer_in_recv], data, data_len);
                    buffer_out_send = buffer_in_recv + data_len;
                    free(data);
                }
                r = buffer_out_send;
            } else {
                ipc_msg_error_abort_t msg_error_abort = {
                    .id = IPC_MSG_ID_ERROR_ABORT,
                    .abort_code = ac,
                };
                buffer_out_send = sizeof(ipc_msg_error_abort_t);
                memcpy(buffer_out, &msg_error_abort, buffer_out_send);
            }
        } else {
            log_error(LOG_RESPONDER "unexpected length for sdo read message: %d", buffer_in_recv);
        }
    }
    return r;
}

static int responder_sdo_write(uint8_t *buffer_in, int buffer_in_recv, uint8_t *buffer_out, int buffer_out_max_len,
                               CO_t *co) {
    int r = -EINVAL;
    if (!buffer_in || buffer_in_recv < 1 || !buffer_out) {
        if (buffer_in_recv > (int)sizeof(ipc_msg_sdo_t)) {
            ipc_msg_sdo_t msg_sdo;
            memcpy(&msg_sdo, buffer_in, sizeof(ipc_msg_sdo_t));
            log_debug(LOG_RESPONDER "sdo write node 0x%X index 0x%X subindex 0x%X", msg_sdo.node_id, msg_sdo.index,
                      msg_sdo.subindex);
            CO_SDO_abortCode_t ac = -1;
            if (co->SDOclient) {
                uint8_t *data = &buffer_in[sizeof(ipc_msg_sdo_t)];
                size_t data_len = buffer_in_recv - sizeof(ipc_msg_sdo_t);
                ac = sdo_write(co->SDOclient, msg_sdo.node_id, msg_sdo.index, msg_sdo.subindex, data, data_len);
            }
            int buffer_out_send = 0;
            if (ac == 0) {
                memcpy(buffer_out, buffer_in, buffer_in_recv);
                buffer_out_send = buffer_in_recv;
            } else {
                ipc_msg_error_abort_t msg_error_abort = {
                    .id = IPC_MSG_ID_ERROR_ABORT,
                    .abort_code = ac,
                };
                buffer_out_send = sizeof(ipc_msg_error_abort_t);
                memcpy(buffer_out, &msg_error_abort, buffer_out_send);
            }
            r = buffer_out_send;
        } else {
            log_error(LOG_RESPONDER "unexpected length for sdo write message: %d", buffer_in_recv);
        }
    }
    return r;
}

static int responder_add_file(uint8_t *buffer_in, int buffer_in_recv, uint8_t *buffer_out, int buffer_out_max_len,
                              fcache_t *fread_cache) {
    int r = -EINVAL;
    if (!buffer_in || buffer_in_recv < 1 || !buffer_out) {
        if (fread_cache == NULL) {
            log_error(LOG_RESPONDER "add fread cache is null");
        } else if (buffer_in_recv > 2) {
            if (buffer_in[buffer_in_recv - 1] != '\0') {
                buffer_in[buffer_in_recv] = '\0';
                buffer_in_recv++;
            }
            int buffer_out_send = 0;
            int error = fcache_add(fread_cache, (char *)&buffer_in[1], false);
            if (error) {
                buffer_out_send = buffer_in_recv;
                memcpy(buffer_out, buffer_in, buffer_out_send);
            } else {
                ipc_msg_error_t msg_error = {
                    .id = IPC_MSG_ID_ERROR_ABORT,
                    .error = error,
                };
                buffer_out_send = sizeof(ipc_msg_error_t);
                memcpy(buffer_out, &msg_error, buffer_out_send);
            }
            r = buffer_out_send;
        } else {
            log_error(LOG_RESPONDER "unexpected length for add file message: %d", buffer_in_recv);
        }
    }
    return r;
}

static int responder_sdo_read_file(uint8_t *buffer_in, int buffer_in_recv, uint8_t *buffer_out, int buffer_out_max_len,
                                   CO_t *co) {
    int r = -EINVAL;
    if (!buffer_in || buffer_in_recv < 1 || !buffer_out) {
        CO_SDO_abortCode_t ac = 0;
        uint8_t node_id = buffer_in[1];
        buffer_in[buffer_in_recv - 1] = 0; // add trailing '\0'

        size_t remote_file_path_offset = 2;
        char *remote_file_path = (char *)&buffer_in[remote_file_path_offset];
        size_t remote_file_path_maxlen = buffer_in_recv - remote_file_path_offset;
        size_t remote_file_path_len = strnlen(remote_file_path, remote_file_path_maxlen) + 1;
        if ((remote_file_path_len == 1) || (remote_file_path_len == remote_file_path_maxlen + 1)) {
            log_error(LOG_RESPONDER "sdo read file had no remote path");
            ac = -1;
        }

        uint32_t local_file_path_offset = remote_file_path_offset + remote_file_path_len;
        char *local_file_path = (char *)&buffer_in[local_file_path_offset];
        size_t local_file_path_maxlen = buffer_in_recv - remote_file_path_len;
        size_t local_file_path_len = strnlen(remote_file_path, local_file_path_maxlen) + 1;
        if ((local_file_path_len == 1) || (local_file_path_len == remote_file_path_maxlen + 1)) {
            log_error(LOG_RESPONDER "sdo read file had no local path");
            ac = -1;
        }

        log_debug(LOG_RESPONDER "sdo read file for node 0x%X from %s -> %s", node_id, remote_file_path,
                  local_file_path);

        if ((ac != 0) && (co->SDOclient)) {

            ac = sdo_write_str(co->SDOclient, node_id, OD_INDEX_FREAD_CACHE, OD_SUBINDEX_FREAD_CACHE_FILE_NAME,
                               remote_file_path);
        }

        int buffer_out_send;
        if (ac != 0) {
            ipc_msg_error_abort_t msg_error_abort = {
                .id = IPC_MSG_ID_ERROR_ABORT,
                .abort_code = ac,
            };
            buffer_out_send = sizeof(ipc_msg_error_abort_t);
            memcpy(buffer_out, &msg_error_abort, buffer_out_send);
            r = buffer_out_send;
        }

        if ((ac != 0) && (co->SDOclient)) {
            ac = sdo_read_to_file(co->SDOclient, node_id, OD_INDEX_FREAD_CACHE, OD_SUBINDEX_FREAD_CACHE_FILE_DATA,
                                  local_file_path);
        }

        if (ac != 0) {
            ipc_msg_error_abort_t msg_error_abort = {
                .id = IPC_MSG_ID_ERROR_ABORT,
                .abort_code = ac,
            };
            buffer_out_send = sizeof(ipc_msg_error_abort_t);
            memcpy(buffer_out, &msg_error_abort, buffer_out_send);
            r = buffer_out_send;
        }
    }
    return r;
}

static int responder_sdo_write_file(uint8_t *buffer_in, int buffer_in_recv, uint8_t *buffer_out, int buffer_out_max_len,
                                    CO_t *co) {
    int r = -EINVAL;
    if (!buffer_in || buffer_in_recv < 1 || !buffer_out) {
        CO_SDO_abortCode_t ac = 0;
        uint8_t node_id = buffer_in[1];
        buffer_in[buffer_in_recv - 1] = 0; // add trailing '\0'

        size_t remote_file_path_offset = 2;
        char *remote_file_path = (char *)&buffer_in[remote_file_path_offset];
        size_t remote_file_path_maxlen = buffer_in_recv - remote_file_path_offset;
        size_t remote_file_path_len = strnlen(remote_file_path, remote_file_path_maxlen) + 1;
        if ((remote_file_path_len == 1) || (remote_file_path_len == remote_file_path_maxlen + 1)) {
            log_error(LOG_RESPONDER "sdo write file had no remote path");
            ac = -1;
        }

        uint32_t local_file_path_offset = remote_file_path_offset + remote_file_path_len;
        char *local_file_path = (char *)&buffer_in[local_file_path_offset];
        size_t local_file_path_maxlen = buffer_in_recv - remote_file_path_len;
        size_t local_file_path_len = strnlen(remote_file_path, local_file_path_maxlen) + 1;
        if ((local_file_path_len == 1) || (local_file_path_len == remote_file_path_maxlen + 1)) {
            log_error(LOG_RESPONDER "sdo write file had no local path");
            ac = -1;
        }

        log_debug(LOG_RESPONDER "sdo read file for node 0x%X from %s -> %s", node_id, remote_file_path,
                  local_file_path);

        if ((ac != 0) && (co->SDOclient)) {

            ac = sdo_write_str(co->SDOclient, node_id, OD_INDEX_FWRITE_CACHE, OD_SUBINDEX_FREAD_CACHE_FILE_NAME,
                               remote_file_path);
        }

        if ((ac != 0) && (co->SDOclient)) {
            ac = sdo_write_from_file(co->SDOclient, node_id, OD_INDEX_FWRITE_CACHE, OD_SUBINDEX_FREAD_CACHE_FILE_DATA,
                                     local_file_path);
        }

        if (ac != 0) {
            ipc_msg_error_abort_t msg_error_abort = {
                .id = IPC_MSG_ID_ERROR_ABORT,
                .abort_code = ac,
            };
            int buffer_out_send = sizeof(ipc_msg_error_abort_t);
            memcpy(buffer_out, &msg_error_abort, buffer_out_send);
            r = buffer_out_send;
        }
    }
    return r;
}

int ipc_responder_files_list(uint8_t *buffer_in, int buffer_in_recv, uint8_t *buffer_out, int buffer_out_max_len,
                             CO_t *co) {
    int r = -EINVAL;
    if (!buffer_in || buffer_in_recv < 1 || !buffer_out) {
        uint8_t node_id = buffer_in[1];
        log_debug(LOG_RESPONDER "sdo list file for node 0x%X", node_id);
        void *data = NULL;
        size_t data_len = 0;
        int buffer_out_send = 0;
        CO_SDO_abortCode_t ac = -1;
        if (co->SDOclient) {
            ac = sdo_read_dynamic(co->SDOclient, node_id, OD_INDEX_FREAD_CACHE, OD_SUBINDEX_FREAD_CACHE_FILES_JSON,
                                  &data, &data_len, false);
        }
        if (ac == 0) {
            if (data == NULL) {
                ipc_msg_error_abort_t msg_error_abort = {
                    .id = IPC_MSG_ID_ERROR_ABORT,
                    .abort_code = CO_SDO_AB_NO_DATA,
                };
                buffer_out_send = sizeof(ipc_msg_error_abort_t);
                memcpy(buffer_out, &msg_error_abort, buffer_out_send);
            } else if ((buffer_in_recv + data_len) > (unsigned int)buffer_out_max_len) {
                free(data);
                ipc_msg_error_abort_t msg_error_abort = {
                    .id = IPC_MSG_ID_ERROR_ABORT,
                    .abort_code = CO_SDO_AB_DATA_LONG,
                };
                buffer_out_send = sizeof(ipc_msg_error_abort_t);
                memcpy(buffer_out, &msg_error_abort, buffer_out_send);
            } else {
                memcpy(buffer_out, buffer_in, buffer_in_recv);
                memcpy(&buffer_out[buffer_in_recv], data, data_len);
                buffer_out_send = buffer_in_recv + data_len;
                free(data);
            }
        } else {
            ipc_msg_error_abort_t msg_error_abort = {
                .id = IPC_MSG_ID_ERROR_ABORT,
                .abort_code = ac,
            };
            buffer_out_send = sizeof(ipc_msg_error_abort_t);
            memcpy(buffer_out, &msg_error_abort, buffer_out_send);
        }
    }
    return r;
}
