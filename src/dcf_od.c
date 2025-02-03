#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#define CO_PROGMEM
#define OD_DEFINITION
#include "301/CO_ODinterface.h"
#include "logger.h"
#include "str2buf.h"
#include "ipc_can_event.h"
#include "dcf_od.h"

#define VARIABLE 0x7
#define ARRAY 0x8
#define RECORD 0x9

#define BOOLEAN 0x1
#define INTERGER8 0x2
#define INTERGER16 0x3
#define INTERGER32 0x4
#define UNSIGNED8 0x5
#define UNSIGNED16 0x6
#define UNSIGNED32 0x7
#define REAL32 0x8
#define VISIBLE_STRING 0x9
#define OCTET_STRING 0xA
#define DOMAIN 0xF
#define REAL64 0x11
#define INTERGER64 0x15
#define UNSIGNED64 0x1B

#define INDEX 1
#define SUBINDEX 2

struct tmp_data_t {
    int object_type;
    int data_type;
    char access_type[20];
    int pdo_mapping;
    int total_subindexes;
    uint16_t index;
    uint8_t subindex;
    char default_value[1024];
    float high_limit;
    float low_limit;
};

static void reset_tmp_data(struct tmp_data_t *data);
static int parse_index_header(const char *line, uint16_t *index);
static int parse_subindex_header(const char *line, uint16_t *index, uint8_t *subindex);
static int fill_var(struct tmp_data_t *data, void **value, OD_size_t *value_length, uint8_t *attribute);
static int fill_entry_index(OD_entry_t *entry, struct tmp_data_t *data);
static int fill_entry_subindex(OD_entry_t *entry, struct tmp_data_t *data, int sub_offset);
static bool parse_int_key(const char *string, int *value);
static bool parse_float_key(const char *string, float *value);
static uint8_t get_acces_attr(char *access_type);

int dcf_od_load(const char *file_path, OD_t **od) {
    if (!file_path || !od) {
        return -1;
    }

    OD_t *out;
    uint32_t size = 0;
    OD_entry_t *od_list = NULL;

    OD_entry_t entry_1018;
    bool entry_1018_flag = false;

    unsigned int e = 0;
    int node_id = 0;

    struct tmp_data_t data;
    reset_tmp_data(&data);

    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    int r;
    int section = 0;

    FILE *fp = fopen(file_path, "r");
    if (fp == NULL) {
        return -1;
    }

    out = malloc(sizeof(OD_t));
    if (out == NULL) {
        log_error("OD_t malloc failed");
        goto error;
    }
    out->list = NULL;
    out->size = 0;

    OD_entry_t *entry;
    int sub_offset = 0;

    int l = -1;
    while ((nread = getline(&line, &len, fp)) != -1) {
        l++;

        if ((line[0] == '\n') || (line[0] == ';')) {
            continue; // empty line or comment
        }

        if (line[0] == '[') {
            // new section add last index/subindex section
            if (section == INDEX) {
                r = fill_entry_index(entry, &data);
                if (r < 0) {
                  log_error("failed to fill index 0x%X at entry %d", data.index, e);
                  goto error;
                }
                sub_offset = 0;
            } else if (section == SUBINDEX) {
                r = fill_entry_subindex(entry, &data, sub_offset);
                if (r < 0) {
                  log_error("failed to fill index 0x%X subindex 0x%X at entry %d", data.index, data.subindex, e);
                  goto error;
                }
                sub_offset++;
            }

            if (e > size) {
                log_error("invalid entry entry %d > size %d", e, size);
                goto error;
            }

            reset_tmp_data(&data);
            section = 0;

            if ((nread == 7) && (line[5] == ']')) {
                if (parse_index_header(line, &data.index)) {
                    log_error("invalid index header line: %d - %s", l, line);
                    goto error;
                }
                section = INDEX;
                data.subindex = 0;
                if ((data.index != 0x1000) && (data.index != 0x1018)) { // 0x1000 is the first entry
                    e++;
                }

                entry = &od_list[e];

                // deal with getting index 0x1018, before 0x1002 -> 0x10017
                // every other index can be assumed to be in order
                if (data.index == 0x1018) {
                    entry = &entry_1018;
                    entry_1018_flag = true;
                } else if(entry_1018_flag && data.index > 0x1018) {
                    entry_1018_flag = false;
                    memcpy(entry, &entry_1018, sizeof(OD_entry_t));
                    e++;
                    entry = &od_list[e];
                }
            } else if (!strncmp(&line[5], "sub", 3) &&
                       (((nread == 11) && (line[9] == ']')) ||
                        ((nread == 12) && (line[10] == ']')))) {
                if (parse_subindex_header(line, &data.index, &data.subindex)) {
                    log_error("invalid subindex header line: %d - %s", l, line);
                    goto error;
                }
                section = SUBINDEX;
            }

            continue; // ignore headers
        }

        if (!strncmp(line, "SupportedObjects=", strlen("SupportedObjects="))) {
            int tmp = 0;
            if (parse_int_key(&line[strlen("SupportedObjects=")], &tmp)) {
                size += (uint32_t)tmp;
                void *p = NULL;
                // size + 1 is for the entry sentinal
                p = (OD_entry_t *)realloc(od_list, sizeof(OD_entry_t) * (size + 1));
                if (p == NULL) {
                  log_error("entry realloc failed: %d", -errno);
                  goto error;
                }
                od_list = p;
                if (size == 3) { // first manitory objects
                    memset(od_list, 0, sizeof(OD_entry_t) * (size + 1));
                } else {
                    memset(&od_list[e + 1], 0, sizeof(OD_entry_t) * tmp);
                }
            }
        } else if (!strncmp(line, "NodeID=", strlen("NodeID="))) {
            parse_int_key(&line[strlen("NodeID=")], &node_id);
        } else if (!strncmp(line, "ObjectType=", strlen("ObjectType="))) {
            parse_int_key(&line[strlen("ObjectType=")], &data.object_type);
        } else if (!strncmp(line, "DataType=", strlen("DataType="))) {
            parse_int_key(&line[strlen("DataType=")], &data.data_type);
        } else if (!strncmp(line, "AccessType=", strlen("AccessType="))) {
            char *tmp = &line[strlen("AccessType=")];
            strncpy(data.access_type, tmp, strlen(tmp));
            data.access_type[strlen(tmp)-1] = '\0';  // remove newline
        } else if (!strncmp(line, "PDOMapping=", strlen("PDOMapping"))) {
            parse_int_key(&line[strlen("PDOMapping=")], &data.pdo_mapping);
        } else if (!strncmp(line, "DefaultValue=", strlen("DefaultValue="))) {
            char *tmp = &line[strlen("DefaultValue=")];
            strncpy(data.default_value, tmp, strlen(tmp));
            data.default_value[strlen(tmp)-1] = '\0';  // remove newline
        } else if (!strncmp(line, "SubNumber=", strlen("SubNumber="))) {
            parse_int_key(&line[strlen("SubNumber=")], &data.total_subindexes);
        } else if (!strncmp(line, "HighLimit=", strlen("HighLimit="))) {
            parse_float_key(&line[strlen("HighLimit=")], &data.high_limit);
        } else if (!strncmp(line, "LowLimit=", strlen("LowLimit="))) {
            parse_float_key(&line[strlen("LowLimit=")], &data.low_limit);
        }
    }

    // add final index/subindex section
    if (section == INDEX) {
        r = fill_entry_index(entry, &data);
        if (r < 0) {
          log_error("failed to fill index 0x%X at entry %d", data.index, e);
          goto error;
        }
    } else if (section == SUBINDEX) {
        r = fill_entry_subindex(entry, &data, sub_offset);
        if (r < 0) {
          log_error("failed to fill index 0x%X subindex 0x%X at entry %d", data.index, data.subindex, e);
          goto error;
        }
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
        dcf_od_free(out);
    }
    return -1;
}

void dcf_od_free(OD_t *od) {
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
    for (int i=0; od->list[i].index != 0; i++) {
        entry = &od->list[i];
        if ((entry == NULL) || (entry->odObject == NULL)) {
            continue;
        }

        if (entry->index >= 0x4000) {
            if (entry->extension && entry->extension->object) {
                ipc_can_event_t *ipc_can_event = (ipc_can_event_t *)&entry->extension->object;
                if (ipc_can_event) {
                    if (ipc_can_event->buffer) {
                        free(ipc_can_event->buffer);
                    }
                    free(ipc_can_event);
                }
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
            for (int j=0; j<entry->subEntriesCount; j++) {
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

static bool parse_float_key(const char *string, float *value) {
    int r;
    float tmp;
    r = sscanf(string, "%f", &tmp);
    if (r == 1) {
        *value = tmp;
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

    if (entry->index < 0x4000) {
        entry->extension = NULL;
    } else {
        OD_extension_t *ext = malloc(sizeof(OD_extension_t));
        if (ext) {
            ipc_can_event_t *ipc_can_event = malloc(sizeof(ipc_can_event_t));
            if (ipc_can_event) {
                memset(ipc_can_event, 0, sizeof(ipc_can_event_t));
                ipc_can_event->dtype = data->data_type;
                ipc_can_event->high_limit = data->high_limit;
                ipc_can_event->low_limit = data->low_limit;
                ext->object = ipc_can_event;
                ext->read = OD_readOriginal;
                ext->write = ipc_can_event_write_cb;
                OD_extension_init(entry, ext);
            } else {
                free(ext);
            }
        }
    }

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
        arr->dataElementLength = data->total_subindexes - 1; // don't include sub0
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
        log_error("invalid object type for index 0x%X subindex 0x%X in dcf: %d", data->index, data->subindex, data->object_type);
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
                case VISIBLE_STRING:
                {
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
                        memset(arr->dataOrig, 0,  size);
                    }
                    arr->attribute = get_acces_attr(data->access_type) | ODA_MB;
                    arr->dataElementSizeof = 0;
                    break;
                default:
                {
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
    data->pdo_mapping = 0;
    data->index = 0;
    data->subindex = 0;
    data->total_subindexes = 0;
    data->default_value[0] = 0;
    data->high_limit = 0.0;
    data->low_limit = 0.0;
}

static int parse_index_header(const char *line, uint16_t *index) {
    char *end;
    char tmp[] = "0000";
    strncpy(tmp, &line[1], 4);
    *index = (uint16_t)strtoul(tmp, &end, 16);
    if (tmp == end) {
        return -1;
    }
    return 0;
}

static int parse_subindex_header(const char *line, uint16_t *index, uint8_t *subindex) {
    char *end;
    char tmp[] = "0000";
    strncpy(tmp, &line[1], 4);
    *index = (uint16_t)strtoul(tmp, &end, 16);
    if (tmp == end) {
        log_error("index line error: '%s'\n", tmp);
        return -1;
    }
    strncpy(tmp, &line[7], 2);
    tmp[0] = line[8];
    if (line[9] == ']') {
        tmp[1] = '\0';
    } else {
        tmp[1] = line[9];
        tmp[2] = '\0';
    }
    *subindex = strtoul(tmp, &end, 16);
    if (tmp == end) {
        return -1;
    }
    return 0;
}

static int fill_var(struct tmp_data_t *data, void **value, OD_size_t *value_length, uint8_t *attribute) {
    OD_size_t length = 0;

    switch (data->data_type) {
    case BOOLEAN:
        *value = (void *)str2buf_bool(data->default_value);
        length = 1;
        break;
    case INTERGER8:
        *value = (void *)str2buf_int8(data->default_value);
        length = 1;
        break;
    case UNSIGNED8:
        *value = (void *)str2buf_uint8(data->default_value);
        length = 1;
        break;
    case INTERGER16:
        *value = (void *)str2buf_int16(data->default_value);
        length = 2;
        *attribute |= ODA_MB;
        break;
    case UNSIGNED16:
        *value = (void *)str2buf_uint16(data->default_value);
        length = 2;
        *attribute |= ODA_MB;
        break;
    case INTERGER32:
        *value = (void *)str2buf_int32(data->default_value);
        length = 4;
        *attribute |= ODA_MB;
        break;
    case UNSIGNED32:
        *value = (void *)str2buf_uint32(data->default_value);
        length = 4;
        *attribute |= ODA_MB;
        break;
    case REAL32:
        *value = (void *)str2buf_float32(data->default_value);
        length = 4;
        *attribute |= ODA_MB;
        break;
    case INTERGER64:
        *value = (void *)str2buf_int64(data->default_value);
        length = 8;
        *attribute |= ODA_MB;
        break;
    case UNSIGNED64:
        *value = (void *)str2buf_uint64(data->default_value);
        length = 8;
        *attribute |= ODA_MB;
        break;
    case REAL64:
        *value = (void *)str2buf_float64(data->default_value);
        length = 8;
        *attribute |= ODA_MB;
        break;
    case VISIBLE_STRING:
        length = strlen(data->default_value) + 1;
        *value = malloc(length);
        strncpy(*value, data->default_value, length);
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

    if (!((data->data_type == DOMAIN) || (data->data_type == OCTET_STRING) || (data->data_type == VISIBLE_STRING ))) {
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

    if (value_length != NULL ) {
        *value_length = length;
    }

    *attribute |= get_acces_attr(data->access_type);

    if (data->pdo_mapping) {
        *attribute |= ODA_TRPDO;
    }

    return 0;
}

static uint8_t get_acces_attr(char *access_type) {
    uint8_t attr;
    if (!strncpy(access_type, "ro", strlen(access_type)) ||
        !strncpy(access_type, "const", strlen(access_type))) {
        attr = ODA_SDO_R;
    } else if (!strncpy(access_type, "wo", strlen(access_type))) {
        attr = ODA_SDO_W;
    } else {
        attr = ODA_SDO_RW;
    }
    return attr;
}
