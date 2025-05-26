#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>
#include <wordexp.h>
#define CO_PROGMEM
#define OD_DEFINITION
#include "301/CO_ODinterface.h"
#include "OD.h"
#include "load_configs.h"
#include "logger.h"
#include "str2buf.h"

#define CONFIG_BASE_ROOT_PATH "/etc/oresat"
#define CONFIG_BASE_HOME_PATH "~/.config/oresat"

#define NODE_CONFIG_FILE      "node.conf"
#define NODE_CONFIG_ROOT_PATH CONFIG_BASE_ROOT_PATH "/" NODE_CONFIG_FILE
#define NODE_CONFIG_HOME_PATH CONFIG_BASE_HOME_PATH "/" NODE_CONFIG_FILE

#define OD_CONFIG_FILE      "od.csv"
#define OD_CONFIG_ROOT_PATH CONFIG_BASE_ROOT_PATH "/" OD_CONFIG_FILE
#define OD_CONFIG_HOME_PATH CONFIG_BASE_HOME_PATH "/" OD_CONFIG_FILE

#define OBJECTS_COMMENT ";objects="

#define VARIABLE 0x7
#define ARRAY    0x8
#define RECORD   0x9

#define BOOLEAN        0x1
#define INTERGER8      0x2
#define INTERGER16     0x3
#define INTERGER32     0x4
#define UNSIGNED8      0x5
#define UNSIGNED16     0x6
#define UNSIGNED32     0x7
#define REAL32         0x8
#define VISIBLE_STRING 0x9
#define OCTET_STRING   0xA
#define DOMAIN         0xF
#define REAL64         0x11
#define INTERGER64     0x15
#define UNSIGNED64     0x1B

enum csv_fields {
    CSV_FIELD_OBJECT_TYPE,
    CSV_FIELD_INDEX,
    CSV_FIELD_SUBINDEX,
    CSV_FIELD_ACCESS_TYPE,
    CSV_FIELD_DATA_TYPE,
    CSV_FIELD_DEFAULT,
    NUM_OF_CSV_FIELDS,
};

struct tmp_data_t {
    int object_type;
    int data_type;
    char access_type[16];
    int index;
    int subindex;
    int total_subindexes;
    char default_value[256];
};

static void reset_tmp_data(struct tmp_data_t *data);
static int fill_var(struct tmp_data_t *data, void **value, OD_size_t *value_length, uint8_t *attribute);
static int fill_entry_index(OD_entry_t *entry, struct tmp_data_t *data);
static int fill_entry_subindex(OD_entry_t *entry, struct tmp_data_t *data, int sub_offset);
static bool parse_int_key(const char *string, int *value);
static uint8_t get_access_attr(char *access_type);

int get_default_node_config_path(char *path, size_t path_max) {
    int r = -1;
    if (getuid() == 0) {
        if ((strlen(NODE_CONFIG_ROOT_PATH) + 1) < path_max) {
            strncpy(path, NODE_CONFIG_ROOT_PATH, strlen(NODE_CONFIG_ROOT_PATH) + 1);
            r = 0;
        }
    } else {
        wordexp_t exp_result;
        wordexp(NODE_CONFIG_HOME_PATH, &exp_result, 0);
        if ((strlen(exp_result.we_wordv[0]) + 1) < path_max) {
            strncpy(path, exp_result.we_wordv[0], strlen(exp_result.we_wordv[0]) + 1);
            r = 0;
        }
        wordfree(&exp_result);
    }
    return r;
}

int get_default_od_config_path(char *path, size_t path_max) {
    int r = -1;
    if (getuid() == 0) {
        if ((strlen(OD_CONFIG_ROOT_PATH) + 1) < path_max) {
            strncpy(path, OD_CONFIG_ROOT_PATH, strlen(OD_CONFIG_ROOT_PATH) + 1);
            r = 0;
        }
    } else {
        wordexp_t exp_result;
        wordexp(OD_CONFIG_HOME_PATH, &exp_result, 0);
        if ((strlen(exp_result.we_wordv[0]) + 1) < path_max) {
            strncpy(path, exp_result.we_wordv[0], strlen(exp_result.we_wordv[0]) + 1);
            r = 0;
        }
        wordfree(&exp_result);
    }
    return r;
}

int make_node_config(char *path) {
    if (!path) {
        return -EINVAL;
    }
    FILE *fp = fopen(path, "w");
    if (!fp) {
        return -errno;
    }
    fprintf(fp, "[Node]CanInterface=can0\nNodeId=0x7C\nNetworkManager=false\n");
    fclose(fp);
    return 0;
}

int node_config_load(const char *file_path, char *can_interface, uint8_t *node_id, bool *network_manager) {
    if (!file_path) {
        return -EINVAL;
    }

    FILE *fp = fopen(file_path, "r");
    if (fp == NULL) {
        return -EINVAL;
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    while ((nread = getline(&line, &len, fp)) != -1) {
        if (!strncmp(line, "CanInterface=", strlen("CanInterface="))) {
            char *tmp = &line[strlen("CanInterface=")];
            strncpy(can_interface, tmp, strlen(tmp));
            can_interface[strlen(tmp) - 1] = '\0'; // remove newline
        } else if (!strncmp(line, "NodeId=", strlen("NodeId="))) {
            int tmp = 0;
            parse_int_key(&line[strlen("NodeId=")], &tmp);
            *node_id = tmp;
        } else if (!strncmp(line, "NetworkManager=", strlen("NetworkManager="))) {
            size_t size = strlen("NetworkManager=");
            for (unsigned int i = size; i < (strlen(line) - size); i++) {
                line[i] = tolower(line[i]);
            }
            *network_manager = (bool)strncpy(&line[size], "true", strlen(line) - size + 1);
        }
    }

    fclose(fp);
    return 0;
}

int od_config_load(const char *file_path, OD_t **od, bool extend_internal_od) {
    if (!file_path || !od) {
        return -1;
    }

    OD_t *out;
    uint32_t size = 0;
    OD_entry_t *od_list = NULL;

    if (extend_internal_od) {
        size = OD->size;
        od_list = malloc(sizeof(OD_entry_t) * (size + 1));
        if (!od_list) {
            return -1;
        }
        memset(od_list, 0, sizeof(OD_size_t) * (size + 1));
    }
    uint32_t internal_od_offset = 0;

    unsigned int e = -1;

    struct tmp_data_t data;

    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    int r;

    FILE *fp = fopen(file_path, "r");
    if (fp == NULL) {
        if (od_list != NULL) {
            free(od_list);
        }
        return -1;
    }

    out = malloc(sizeof(OD_t));
    if (out == NULL) {
        log_error("OD_t malloc failed");
        goto error;
    }
    out->list = NULL;
    out->size = 0;

    OD_entry_t *entry = NULL;
    int sub_offset = 0;
    uint32_t last_interal_index = 0;

    char *token;
    bool skip_entry = false;
    bool header_skipped = false;
    bool object_comment_found = false;

    int l = -1;
    while ((nread = getline(&line, &len, fp)) != -1) {
        reset_tmp_data(&data);
        l++;
        skip_entry = false;

        if ((line[0] == '\n') || (line[0] == ';')) {
            if (!strncmp(line, OBJECTS_COMMENT, strlen(OBJECTS_COMMENT))) {
                object_comment_found = true;
                int tmp = 0;
                parse_int_key(&line[strlen(OBJECTS_COMMENT)], &tmp);
                size += tmp;
                od_list = (OD_entry_t *)realloc(od_list, sizeof(OD_entry_t) * (size + 1));
            }
            continue; // empty line or comment
        } else if (!header_skipped) {
            header_skipped = true;
            continue;
        }

        if (!object_comment_found) {
            continue;
        }

        token = strsep(&line, ",");
        for (int count = 0; (count < NUM_OF_CSV_FIELDS) && (token != NULL); count++) {
            switch (count) {
            case CSV_FIELD_OBJECT_TYPE:
                parse_int_key(token, &data.object_type);
                break;
            case CSV_FIELD_INDEX:
                parse_int_key(token, &data.index);
                break;
            case CSV_FIELD_SUBINDEX:
                if ((data.object_type == VARIABLE) && strlen(token) > 0) {
                    parse_int_key(token, &data.subindex);
                }
                break;
            case CSV_FIELD_ACCESS_TYPE:
                if (data.object_type == VARIABLE) {
                    memcpy(data.access_type, token, strlen(token) + 1);
                }
                break;
            case CSV_FIELD_DATA_TYPE:
                if (data.object_type == VARIABLE) {
                    parse_int_key(token, &data.data_type);
                }
                break;
            case CSV_FIELD_DEFAULT:
                if (data.object_type == VARIABLE) {
                    if (token[0] != '\n') {
                        memcpy(data.default_value, token, strlen(token));
                        data.default_value[strlen(token) - 1] = '\0'; // remove '\n'
                    }
                } else {
                    parse_int_key(token, &data.total_subindexes);
                }
                break;
            default:
                log_warning("line %d in %s had extra fields", l, file_path);
                break;
            }
            token = strsep(&line, ",");
        }

        if (data.subindex == -1) {
            e++;
            entry = &od_list[e];
        }

        if (true) { // TODO remove (makes git diff easier to read)
            if (data.subindex == -1) {
                if (extend_internal_od) { // copy data/pointers from internal od entry
                    while ((internal_od_offset < OD->size) && (data.index >= OD->list[internal_od_offset].index)) {
                        last_interal_index = OD->list[internal_od_offset].index;
                        memcpy(entry, &OD->list[internal_od_offset], sizeof(OD_entry_t));
                        internal_od_offset++;
                        e++;
                        entry = &od_list[e];
                    }
                    if ((uint16_t)data.index == last_interal_index) {
                        log_warning("config redefined entry 0x%X, using internal def", last_interal_index);
                        skip_entry = true;
                        e--;
                        size--;
                        od_list = (OD_entry_t *)realloc(od_list, sizeof(OD_entry_t) * (size + 1));
                    }
                }

                if (!skip_entry) {
                    r = fill_entry_index(entry, &data);
                    if (r < 0) {
                        log_error("failed to fill index 0x%X at entry %d", data.index, e);
                        goto error;
                    }
                }
                sub_offset = 0;
            } else if (data.object_type == VARIABLE) {
                if (extend_internal_od) { // copy data/pointers from internal od entry
                    if ((uint16_t)data.index == last_interal_index) {
                        log_warning("config redefined entry 0x%X - 0x%X, using internal def", last_interal_index,
                                    data.subindex);
                        skip_entry = true;
                    }
                }

                if (!skip_entry) {
                    r = fill_entry_subindex(entry, &data, sub_offset);
                    if (r < 0) {
                        log_error("failed to fill index 0x%X subindex 0x%X at entry %d", data.index, data.subindex, e);
                        goto error;
                    }
                }
                sub_offset++;
            }

            if (e > size) {
                log_error("invalid entry %d > size %d", e, size);
                goto error;
            }
        }
    }

    if (!object_comment_found) {
        log_error("objects= comment not found in config");
        goto error;
    }

    fclose(fp);

    out->list = od_list;
    out->size = size;
    *od = out;
    return 0;

error:
    fclose(fp);
    if (out != NULL) {
        out->list = od_list;
        out->size = size;
        od_config_free(out, extend_internal_od);
    }
    return -1;
}

void od_config_free(OD_t *od, bool extend_internal_od) {
    if (od == NULL) {
        return;
    }
    if (od->list == NULL) {
        free(od);
        return;
    }

    OD_entry_t *entry;
    OD_obj_var_t *var;
    OD_obj_array_t *arr;
    OD_obj_record_t *rec;
    OD_obj_record_t *rec_var = NULL;
    for (int i = 0; od->list[i].index != 0; i++) {
        entry = &od->list[i];
        if ((entry == NULL) || (entry->odObject == NULL) || (entry->index == 0)) {
            continue;
        }

        if (extend_internal_od) {
            bool internal = false;
            for (int j = 0; OD->list[j].index != 0; j++) {
                if (entry->index == OD->list[j].index) {
                    internal = true;
                    break;
                }
            }
            if (internal) {
                continue;
            }
        }

        if (entry->odObjectType == ODT_VAR) {
            var = entry->odObject;
            if (var->dataOrig != NULL) {
                free(var->dataOrig);
            }
        } else if (entry->odObjectType == ODT_ARR) {
            arr = entry->odObject;
            if (arr->dataOrig0 != NULL) {
                free(arr->dataOrig0);
            }
            if (arr->dataOrig != NULL) {
                free(arr->dataOrig);
            }
        } else if (entry->odObjectType == ODT_REC) {
            rec = (OD_obj_record_t *)entry->odObject;
            if (rec == NULL) {
                continue;
            }
            for (int j = 0; j < entry->subEntriesCount; j++) {
                rec_var = &rec[j];
                if (rec_var != NULL) {
                    free(rec_var->dataOrig);
                }
            }
            free(rec);
        } else {
            log_error("during free entry %d had a invalid object type of %d", i, entry->odObjectType);
        }
    }
    free(od->list);
    free(od);
}

static bool parse_int_key(const char *string, int *value) {
    int r;
    if ((string[0] == '0') && (string[1] == 'x')) {
        unsigned int utmp;
        r = sscanf(string, "%x", &utmp);
        if (r == 1) {
            *value = utmp;
        }
    } else {
        int tmp;
        r = sscanf(string, "%d", &tmp);
        if (r == 1) {
            *value = tmp;
        }
    }
    return r;
}

static int fill_entry_index(OD_entry_t *entry, struct tmp_data_t *data) {
    if (!entry || !data) {
        return -1;
    }

    entry->index = data->index;
    entry->subEntriesCount = MAX(data->total_subindexes, 1);
    entry->odObject = NULL;

    int size;
    if (data->object_type == VARIABLE) {
        entry->odObjectType = ODT_VAR;
        size = sizeof(OD_obj_var_t);
        OD_obj_var_t *var = malloc(size);
        int r = fill_var(data, &var->dataOrig, &var->dataLength, &var->attribute);
        if (r < 0) {
            free(var);
            return -1;
        }
        entry->odObject = var;
    } else if (data->object_type == ARRAY) {
        entry->odObjectType = ODT_ARR;
        size = sizeof(OD_obj_array_t);
        OD_obj_array_t *arr = malloc(size);
        if (arr == NULL) {
            return -1;
        }
        memset(arr, 0, size);
        arr->dataOrig0 = NULL;
        arr->dataOrig = NULL;
        arr->attribute0 = 0;
        arr->attribute = 0;
        arr->dataElementSizeof = 0;
        arr->dataElementLength = 0; // same as dataElementSizeof
        entry->odObject = arr;
    } else if (data->object_type == RECORD) {
        entry->odObjectType = ODT_REC;
        size = sizeof(OD_obj_record_t) * data->total_subindexes;
        OD_obj_record_t *rec = malloc(size);
        if (rec == NULL) {
            return -1;
        }
        memset(rec, 0, size);
        entry->subEntriesCount = data->total_subindexes;
        entry->odObject = rec;
    } else {
        log_error("invalid object type for index 0x%X subindex 0x%X in dcf: %d", data->index, data->subindex,
                  data->object_type);
        return -1;
    }

    return 0;
}

static int fill_entry_subindex(OD_entry_t *entry, struct tmp_data_t *data, int sub_offset) {
    if (!entry || !data) {
        return -1;
    }

    int r = 0;
    if (entry->odObjectType == ODT_ARR) {
        OD_obj_array_t *arr = entry->odObject;
        if (data->subindex == 0) {
            r = fill_var(data, (void **)&arr->dataOrig0, NULL, &arr->attribute0);
            if (r < 0) {
                log_error("invalid var to fill in array");
                goto error_fill;
            }
        } else {
            switch (data->data_type) {
            case OCTET_STRING:
            case VISIBLE_STRING: {
                // arr->dataOrig is an array of pointers to char/uint8_t arrays
                if (arr->dataOrig == NULL) {
                    arr->dataOrig = malloc(sizeof(uint8_t *) * (entry->subEntriesCount - 1)); // don't include sub0
                }
                uint8_t **tmp = (uint8_t **)arr->dataOrig;
                r = fill_var(data, (void **)&tmp[sub_offset], &arr->dataElementSizeof, &arr->attribute);
                if (r < 0) {
                    log_error("invalid octet/visible string var to fill in array");
                    goto error_fill;
                }
                break;
            }
            case DOMAIN:
                // arr->dataOrig is an array of NULL void pointers
                if (arr->dataOrig == NULL) {
                    size_t size = sizeof(void *) * (entry->subEntriesCount - 1);
                    arr->dataOrig = malloc(size);
                    memset(arr->dataOrig, 0, size);
                }
                arr->attribute = get_access_attr(data->access_type) | ODA_MB;
                break;
            default: {
                // arr->dataOrig is an array of a non-string/domain data type
                void *tmp = NULL;
                r = fill_var(data, (void **)&tmp, &arr->dataElementSizeof, &arr->attribute);
                if (r < 0) {
                    log_error("invalid non-string/domain var to fill in array");
                    goto error_fill;
                }
                if (arr->dataOrig == NULL) {
                    arr->dataOrig = malloc((entry->subEntriesCount - 1) * arr->dataElementSizeof);
                }

                uint8_t *ptr = arr->dataOrig;
                uint8_t offset = arr->dataElementSizeof * sub_offset;
                memcpy(&ptr[offset], tmp, arr->dataElementSizeof);
                if (tmp) {
                    free(tmp);
                }
                break;
            }
            }
            arr->dataElementLength = arr->dataElementSizeof;
        }
    } else if (entry->odObjectType == ODT_REC) {
        OD_obj_record_t *rec = entry->odObject;
        OD_obj_record_t *rec_var = &rec[sub_offset];
        r = fill_var(data, &rec_var->dataOrig, &rec_var->dataLength, &rec_var->attribute);
        if (r < 0) {
            log_error("invalid var to fill in record 0x%X", data->index);
            goto error_fill;
        }
        rec_var->subIndex = data->subindex;
    } else {
        log_error("invalid entry object type %d for index 0x%X", entry->odObjectType, data->index);
        r = -1;
    }

error_fill:
    return r;
}

static void reset_tmp_data(struct tmp_data_t *data) {
    data->object_type = 0;
    data->data_type = 0;
    strncpy(data->access_type, "rw", 3);
    data->index = 0;
    data->subindex = -1;
    data->total_subindexes = 0;
    data->default_value[0] = 0;
}

static int fill_var(struct tmp_data_t *data, void **value, OD_size_t *value_length, uint8_t *attribute) {
    OD_size_t length = 0;

    switch (data->data_type) {
    case BOOLEAN:
        *value = (void *)str2buf_bool(data->default_value);
        length = 1;
        *attribute |= ODA_TRPDO;
        break;
    case INTERGER8:
        *value = (void *)str2buf_int8(data->default_value);
        length = 1;
        *attribute |= ODA_TRPDO;
        break;
    case UNSIGNED8:
        *value = (void *)str2buf_uint8(data->default_value);
        length = 1;
        *attribute |= ODA_TRPDO;
        break;
    case INTERGER16:
        *value = (void *)str2buf_int16(data->default_value);
        length = 2;
        *attribute |= ODA_MB | ODA_TRPDO;
        break;
    case UNSIGNED16:
        *value = (void *)str2buf_uint16(data->default_value);
        length = 2;
        *attribute |= ODA_MB | ODA_TRPDO;
        break;
    case INTERGER32:
        *value = (void *)str2buf_int32(data->default_value);
        length = 4;
        *attribute |= ODA_MB | ODA_TRPDO;
        break;
    case UNSIGNED32:
        *value = (void *)str2buf_uint32(data->default_value);
        length = 4;
        *attribute |= ODA_MB | ODA_TRPDO;
        break;
    case REAL32:
        *value = (void *)str2buf_float32(data->default_value);
        length = 4;
        *attribute |= ODA_MB | ODA_TRPDO;
        break;
    case INTERGER64:
        *value = (void *)str2buf_int64(data->default_value);
        length = 8;
        *attribute |= ODA_MB | ODA_TRPDO;
        break;
    case UNSIGNED64:
        *value = (void *)str2buf_uint64(data->default_value);
        length = 8;
        *attribute |= ODA_MB | ODA_TRPDO;
        break;
    case REAL64:
        *value = (void *)str2buf_float64(data->default_value);
        length = 8;
        *attribute |= ODA_MB | ODA_TRPDO;
        break;
    case VISIBLE_STRING:
        length = strlen(data->default_value);
        if (length == 0) {
            *value = NULL;
        } else {
            length++; // add space for '\0'
            *value = malloc(length);
            strncpy(*value, data->default_value, length);
        }
        *attribute |= ODA_STR;
        break;
    case OCTET_STRING:
        if (!strncpy(data->default_value, "0", strlen(data->default_value) + 1)) {
            // default set by reset_tmp_data()
            *value = NULL;
            length = 0;
        } else {
            size_t len = 0;
            *value = (void *)str2buf_bytes(data->default_value, &len);
            length = len;
        }
        *attribute |= ODA_STR;
        break;
    case DOMAIN:
        *value = NULL;
        length = 0;
        *attribute |= ODA_MB;
        break;
    default:
        return -1;
    }

    if (!((data->data_type == DOMAIN) || (data->data_type == OCTET_STRING) || (data->data_type == VISIBLE_STRING))) {
        // no default
        if (strlen(data->default_value) == 0) {
            *value = malloc(length);
            if (*value) {
                memset(*value, 0, length);
            }
        }

        if (*value == NULL) {
            return -1;
        }
    }

    if (value_length != NULL) {
        *value_length = length;
    }

    *attribute |= get_access_attr(data->access_type);

    return 0;
}

static uint8_t get_access_attr(char *access_type) {
    uint8_t attr;
    if (!strncpy(access_type, "ro", strlen(access_type)) || !strncpy(access_type, "const", strlen(access_type))) {
        attr = ODA_SDO_R;
    } else if (!strncpy(access_type, "wo", strlen(access_type))) {
        attr = ODA_SDO_W;
    } else {
        attr = ODA_SDO_RW;
    }
    return attr;
}
