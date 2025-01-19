#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int get_str_base(const char *str) {
    return ((strlen(str) >= 3) && (str[0] == '0') && (str[1] == 'x')) ? 16 : 10;
}

bool* str2buf_bool(const char *str) {
    if (str == NULL || !strlen(str)) return NULL;
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
    if (str == NULL || !strlen(str) || str[0] == '-') return NULL;
    char *endptr;
    unsigned long tmp = strtoul(str, &endptr, get_str_base(str));
    if (tmp > 0xFF || endptr == str) {
        return NULL;
    }
    uint8_t *value = (uint8_t *)malloc(sizeof(uint8_t));
    if (value)
        *value = (uint8_t)tmp;
    return value;
}

uint16_t* str2buf_uint16(const char *str) {
    if (str == NULL || !strlen(str) || str[0] == '-') return NULL;
    char *endptr;
    unsigned long tmp = strtoul(str, &endptr, get_str_base(str));
    if (tmp > 0xFFFF || endptr == str) {
        return NULL;
    }
    uint16_t *value = (uint16_t *)malloc(sizeof(uint16_t));
    if (value)
        *value = (uint16_t)tmp;
    return value;
}

uint32_t* str2buf_uint32(const char *str) {
    if (str == NULL || !strlen(str) || str[0] == '-') return NULL;
    char *endptr;
    unsigned long tmp = strtoul(str, &endptr, get_str_base(str));
    if (endptr == str) {
        return NULL;
    }
    uint32_t *value = (uint32_t *)malloc(sizeof(uint32_t));
    if (value)
        *value = tmp;
    return value;
}

uint64_t* str2buf_uint64(const char *str) {
    if (str == NULL || !strlen(str) || str[0] == '-') return NULL;
    char *endptr;
    unsigned long long tmp = strtoull(str, &endptr, get_str_base(str));
    if (endptr == str) {
        return NULL;
    }
    uint64_t *value = (uint64_t *)malloc(sizeof(uint64_t));
    if (value)
        *value = tmp;
    return value;
}

int8_t* str2buf_int8(const char *str) {
    if (str == NULL || !strlen(str)) return NULL;
    char *endptr;
    long tmp = strtol(str, &endptr, get_str_base(str));
    if (tmp < -0x80 || tmp > 0x7F || endptr == str) {
        return NULL;
    }
    int8_t *value = (int8_t *)malloc(sizeof(int8_t));
    if (value)
        *value = (int8_t)tmp;
    return value;
}

int16_t* str2buf_int16(const char *str) {
    if (str == NULL || !strlen(str)) return NULL;
    char *endptr;
    long tmp = strtol(str, &endptr, get_str_base(str));
    if (tmp < -0x8000 || tmp > 0x7FFF || endptr == str) {
        return NULL;
    }
    int16_t *value = (int16_t *)malloc(sizeof(int16_t));
    if (value)
        *value = (int16_t)tmp;
    return value;
}

int32_t* str2buf_int32(const char *str) {
    if (str == NULL || !strlen(str)) return NULL;
    char *endptr;
    long tmp = strtol(str, &endptr, get_str_base(str));
    if (endptr == str) {
        return NULL;
    }
    int32_t *value = (int32_t *)malloc(sizeof(int32_t));
    if (value)
        *value = (int32_t)tmp;
    return value;
}

int64_t* str2buf_int64(const char *str) {
    if (str == NULL || !strlen(str)) return NULL;
    char *endptr;
    long long tmp = strtoll(str, &endptr, get_str_base(str));
    if (endptr == str) {
        return NULL;
    }
    int64_t *value = (int64_t *)malloc(sizeof(int64_t));
    if (value)
        *value = (int64_t)tmp;
    return value;
}

float* str2buf_float32(const char *str) {
    if (str == NULL || !strlen(str)) return NULL;
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
    if (str == NULL || !strlen(str)) return NULL;
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
    if (!str || !out_len || !strlen(str) || (strlen(str) % 2)) return NULL;
    size_t len = strlen(str) / 2;
    uint8_t *value = (uint8_t *)malloc(len);
    if (value) {
        char *endptr;
        char tmp[] = "00";
        bool valid = true;
        size_t offset = 0;
        for (size_t i=0; i < len; i++) {
            offset = 2*i;
            tmp[0] = str[offset];
            tmp[1] = str[offset + 1];
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

    if (!strncmp(dtype, "bool", strlen(dtype))) {
        buf = (void *)str2buf_bool(str);
        buf_len = 1;
    } else if (!strncmp(dtype, "int8", strlen(dtype))) {
        buf = (void *)str2buf_int8(str);
        buf_len = 1;
    } else if (!strncmp(dtype, "int16", strlen(dtype))) {
        buf = (void *)str2buf_int16(str);
        buf_len = 2;
    } else if (!strncmp(dtype, "int32", strlen(dtype))) {
        buf = (void *)str2buf_int32(str);
        buf_len = 4;
    } else if (!strncmp(dtype, "int64", strlen(dtype))) {
        buf = (void *)str2buf_int64(str);
        buf_len = 8;
    } else if (!strncmp(dtype, "uint8", strlen(dtype))) {
        buf = (void *)str2buf_uint8(str);
        buf_len = 1;
    } else if (!strncmp(dtype, "uint16", strlen(dtype))) {
        buf = (void *)str2buf_uint16(str);
        buf_len = 2;
    } else if (!strncmp(dtype, "uint32", strlen(dtype))) {
        buf = (void *)str2buf_uint32(str);
        buf_len = 4;
    } else if (!strncmp(dtype, "uint64", strlen(dtype))) {
        buf = (void *)str2buf_uint64(str);
        buf_len = 8;
    } else if (!strncmp(dtype, "float32", strlen(dtype))) {
        buf = (void *)str2buf_float32(str);
        buf_len = 4;
    } else if (!strncmp(dtype, "float64", strlen(dtype))) {
        buf = (void *)str2buf_float64(str);
        buf_len = 8;
    } else if (!strncmp(dtype, "string", strlen(dtype))) {
        buf_len = strlen(str);
        buf = (void *)malloc(buf_len + 1);
        memcpy(buf, str, buf_len);
        char *tmp = (char *)buf;
        tmp[buf_len] = '\0';
    } else if (!strncmp(dtype, "bytes", strlen(dtype))) {
        buf = (void *)str2buf_bytes(str, &buf_len);
    }

    if (buf) {
        *out_len = buf_len;
    }
    return buf;
}
