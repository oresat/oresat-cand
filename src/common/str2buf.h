#ifndef _STR2BUF_H_
#define _STR2BUF_H_

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

// all the return values must be freed

bool* str2buf_bool(const char *str);

uint8_t* str2buf_uint8(const char *str);
uint16_t* str2buf_uint16(const char *str);
uint32_t* str2buf_uint32(const char *str);
uint64_t* str2buf_uint64(const char *str);

int8_t* str2buf_int8(const char *str);
int16_t* str2buf_int16(const char *str);
int32_t* str2buf_int32(const char *str);
int64_t* str2buf_int64(const char *str);

float* str2buf_float32(const char *str);
double* str2buf_float64(const char *str);

uint8_t* str2buf_bytes(const char *str, size_t *out_len);

void* str2buf_dtype_int(uint16_t dtype, const char *str, size_t *out_len);
void* str2buf_dtype_str(const char *dtype, const char *str, size_t *out_len);

#endif
