#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int get_str_base(const char *str) {
    return ((strlen(str) >= 3) && (str[0] == '0') && (str[1] == 'x')) ? 16 : 10;
}

bool* str2buf_bool(const char *str) {
    if (str == NULL) return NULL;
    bool *value = (bool *)malloc(sizeof(bool));
    if (value) {
        if (!strncmp(str, "true", strlen(str))
                || !strncmp(str, "1", strlen(str))) {
            *value = true;
        } else if (!strncmp(str, "false", strlen(str))
                || !strncmp(str, "0", strlen(str))) {
            *value = false;
        } else {
            free(value);
            value = NULL;
        }
    }
    return value;
}

uint8_t* str2buf_uint8(const char *str) {
    if (str == NULL) return NULL;
    uint8_t *value = (uint8_t *)malloc(sizeof(uint8_t));
    if (value) {
        char *endptr;
        *value = (uint8_t)strtoul(str, &endptr, get_str_base(str));
        if (endptr == str) {
            free(value);
            value = NULL;
        }
    }
    return value;
}

uint16_t* str2buf_uint16(const char *str) {
    if (str == NULL) return NULL;
    uint16_t *value = (uint16_t *)malloc(sizeof(uint16_t));
    if (value) {
        char *endptr;
        *value = (uint16_t)strtoul(str, &endptr, get_str_base(str));
        if (endptr == str) {
            free(value);
            value = NULL;
        }
    }
    return value;
}

uint32_t* str2buf_uint32(const char *str) {
    if (str == NULL) return NULL;
    uint32_t *value = (uint32_t *)malloc(sizeof(uint32_t));
    if (value) {
        char *endptr;
        *value = strtoul(str, &endptr, get_str_base(str));
        if (endptr == str) {
            free(value);
            value = NULL;
        }
    }
    return value;
}

uint64_t* str2buf_uint64(const char *str) {
    if (str == NULL) return NULL;
    uint64_t *value = (uint64_t *)malloc(sizeof(uint64_t));
    if (value) {
        char *endptr;
        *value = strtoull(str, &endptr, get_str_base(str));
        if (endptr == str) {
            free(value);
            value = NULL;
        }
    }
    return value;
}

int8_t* str2buf_int8(const char *str) {
    if (str == NULL) return NULL;
    int8_t *value = (int8_t *)malloc(sizeof(int8_t));
    if (value) {
        char *endptr;
        *value = (int8_t)strtol(str, &endptr, get_str_base(str));
        if (endptr == str) {
            free(value);
            value = NULL;
        }
    }
    return value;
}

int16_t* str2buf_int16(const char *str) {
    if (str == NULL) return NULL;
    int16_t *value = (int16_t *)malloc(sizeof(int16_t));
    if (value) {
        char *endptr;
        *value = (int16_t)strtol(str, &endptr, get_str_base(str));
        if (endptr == str) {
            free(value);
            value = NULL;
        }
    }
    return value;
}

int32_t* str2buf_int32(const char *str) {
    if (str == NULL) return NULL;
    int32_t *value = (int32_t *)malloc(sizeof(int32_t));
    if (value) {
        char *endptr;
        *value = strtol(str, &endptr, get_str_base(str));
        if (endptr == str) {
            free(value);
            value = NULL;
        }
    }
    return value;
}

int64_t* str2buf_int64(const char *str) {
    if (str == NULL) return NULL;
    int64_t *value = (int64_t *)malloc(sizeof(int64_t));
    if (value) {
        char *endptr;
        *value = strtoll(str, &endptr, get_str_base(str));
        if (endptr == str) {
            free(value);
            value = NULL;
        }
    }
    return value;
}

float* str2buf_float32(const char *str) {
    if (str == NULL) return NULL;
    float *value = (float *)malloc(sizeof(float));
    if (value) {
        char *endptr;
        *value = (float)strtof(str, &endptr);
        if (endptr == str) {
            free(value);
            value = NULL;
        }
    }
    return value;
}

double* str2buf_float64(const char *str) {
    if (str == NULL) return NULL;
    double *value = (double *)malloc(sizeof(double));
    if (value) {
        char *endptr;
        *value = (double)strtof(str, &endptr);
        if (endptr == str) {
            free(value);
            value = NULL;
        }
    }
    return value;
}

uint8_t* str2buf_bytes(const char *str, size_t *out_len) {
    if ((str == NULL) || (out_len == NULL) || (strlen(str) % 2)) return NULL;
    size_t len = strlen(str) / 2;
    uint8_t *value = (uint8_t *)malloc(len);
    if (value) {
        char *endptr;
        char tmp[] = "00";
        bool valid = true;
        for (size_t i=0; i < len; i++) {
            tmp[0] = str[i];
            tmp[1] = str[i + 1];
            value[i] = (uint8_t)strtoul(tmp, &endptr, 16);
            if (endptr == str) {
                valid = false;
                break;
            }
        }
        if (valid) {
            *out_len = len;
        } else {
            free(value);
            value = NULL;
            *out_len = 0;
        }
    }
    return value;
}

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

void* str2buf_dtype_int(uint16_t dtype, const char *str, size_t *out_len) {
    void *buf = NULL;
    size_t buf_len = 0;

    switch (dtype) {
        case BOOLEAN:
            buf = (void *)str2buf_bool(str);
            buf_len = 1;
            break;
        case INTERGER8:
            buf = (void *)str2buf_int8(str);
            buf_len = 1;
            break;
        case INTERGER16:
            buf = (void *)str2buf_int16(str);
            buf_len = 2;
            break;
        case INTERGER32:
            buf = (void *)str2buf_int32(str);
            buf_len = 4;
            break;
        case INTERGER64:
            buf = (void *)str2buf_int64(str);
            buf_len = 8;
            break;
        case UNSIGNED8:
            buf = (void *)str2buf_uint8(str);
            buf_len = 1;
            break;
        case UNSIGNED16:
            buf = (void *)str2buf_uint16(str);
            buf_len = 2;
            break;
        case UNSIGNED32:
            buf = (void *)str2buf_uint32(str);
            buf_len = 4;
            break;
        case UNSIGNED64:
            buf = (void *)str2buf_uint64(str);
            buf_len = 8;
            break;
        case REAL32:
            buf = (void *)str2buf_float32(str);
            buf_len = 4;
            break;
        case REAL64:
            buf = (void *)str2buf_float64(str);
            buf_len = 8;
            break;
        case VISIBLE_STRING:
        {
            buf_len = strlen(str);
            buf = (void *)malloc(buf_len + 1);
            memcpy(buf, str, buf_len);
            char *tmp = (char *)buf;
            tmp[buf_len] = '\0';
            break;
        }
        case OCTET_STRING:
            buf = (void *)str2buf_bytes(str, &buf_len);
        default:
            break;
    }

    if (buf) {
        *out_len = buf_len;
    }
    return buf;
}

void* str2buf_dtype_str(const char *dtype, const char *str, size_t *out_len) {
    void *buf = NULL;
    size_t buf_len = 0;

#define STRNCMP3(str1, str2, str3) \
        (!strncmp(dtype, str1, strlen(dtype)) || \
         !strncmp(dtype, str2, strlen(dtype)) || \
         !strncmp(dtype, str3, strlen(dtype)))

    if (STRNCMP3("b", "bool", "boolean")) {
        buf = (void *)str2buf_bool(str);
        buf_len = 1;
    } else if (STRNCMP3("i8", "int8", "interger8")) {
        buf = (void *)str2buf_int8(str);
        buf_len = 1;
    } else if (STRNCMP3("i16", "int16", "interger16")) {
        buf = (void *)str2buf_int16(str);
        buf_len = 2;
    } else if (STRNCMP3("i32", "int32", "interger32")) {
        buf = (void *)str2buf_int32(str);
        buf_len = 4;
    } else if (STRNCMP3("i64", "int64", "interger64")) {
        buf = (void *)str2buf_int64(str);
        buf_len = 8;
    } else if (STRNCMP3("u8", "uint8", "unsigned8")) {
        buf = (void *)str2buf_uint8(str);
        buf_len = 1;
    } else if (STRNCMP3("u16", "uint16", "unsigned16")) {
        buf = (void *)str2buf_uint16(str);
        buf_len = 2;
    } else if (STRNCMP3("u32", "uint32", "unsigned32")) {
        buf = (void *)str2buf_uint32(str);
        buf_len = 4;
    } else if (STRNCMP3("u64", "uint64", "unsigned64")) {
        buf = (void *)str2buf_uint64(str);
        buf_len = 8;
    } else if (STRNCMP3("f32", "float", "real32")) {
        buf = (void *)str2buf_float32(str);
        buf_len = 4;
    } else if (STRNCMP3("f64", "float32", "real64")) {
        buf = (void *)str2buf_float32(str);
        buf_len = 4;
    } else if (STRNCMP3("s", "str", "visible_string")) {
        buf_len = strlen(str);
        buf = (void *)malloc(buf_len + 1);
        memcpy(buf, str, buf_len);
        char *tmp = (char *)buf;
        tmp[buf_len] = '\0';
    } else if (STRNCMP3("o", "bytes", "octet_string")) {
        buf = (void *)str2buf_bytes(str, &buf_len);
    }

#undef STRNCMP3

    if (buf) {
        *out_len = buf_len;
    }
    return buf;
}
