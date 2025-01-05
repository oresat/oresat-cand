#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "str2buf.h"

#define ASSERT_STR2BUF(dtype, str, value, ptr)\
    ptr = str2buf_##dtype(str);\
    assert((ptr != NULL) && (*ptr == value));\
    free(ptr)

#define ASSERT_STR2BUF_NULL(dtype, str)\
    assert(str2buf_##dtype(str) == NULL)

void test_str2buf_bool(void) {
    bool *ptr;

    ASSERT_STR2BUF(bool, "0", 0, ptr);
    ASSERT_STR2BUF(bool, "1", 1, ptr);
    ASSERT_STR2BUF(bool, "false", 0, ptr);
    ASSERT_STR2BUF(bool, "true", 1, ptr);

    ASSERT_STR2BUF_NULL(bool, "");
    ASSERT_STR2BUF_NULL(bool, "abc");
    ASSERT_STR2BUF_NULL(bool, "afalse");
    ASSERT_STR2BUF_NULL(bool, "truea");
}

void test_str2buf_int8(void) {
    int8_t *ptr;

    ASSERT_STR2BUF(int8, "0", 0, ptr);
    ASSERT_STR2BUF(int8, "1", 1, ptr);
    ASSERT_STR2BUF(int8, "-1", -1, ptr);
    ASSERT_STR2BUF(int8, "-128", -128, ptr);
    ASSERT_STR2BUF(int8, "-127", -127, ptr);

    ASSERT_STR2BUF_NULL(int8, "");
    ASSERT_STR2BUF_NULL(int8, "abc");
    ASSERT_STR2BUF_NULL(int8, "128");
    ASSERT_STR2BUF_NULL(int8, "-129");
}

void test_str2buf_uint8(void) {
    uint8_t *ptr;

    ASSERT_STR2BUF(uint8, "0", 0, ptr);
    ASSERT_STR2BUF(uint8, "0x0", 0, ptr);
    ASSERT_STR2BUF(uint8, "1", 1, ptr);
    ASSERT_STR2BUF(uint8, "255", 255, ptr);
    ASSERT_STR2BUF(uint8, "0xFF", 255, ptr);

    ASSERT_STR2BUF_NULL(uint8, "");
    ASSERT_STR2BUF_NULL(uint8, "abc");
    ASSERT_STR2BUF_NULL(uint8, "256");
    ASSERT_STR2BUF_NULL(uint8, "-1");
}

void test_str2buf_int16(void) {
    int16_t *ptr;

    ASSERT_STR2BUF(int16, "0", 0, ptr);
    ASSERT_STR2BUF(int16, "1", 1, ptr);
    ASSERT_STR2BUF(int16, "-1", -1, ptr);
    ASSERT_STR2BUF(int16, "-32768", -32768, ptr);
    ASSERT_STR2BUF(int16, "32767", 32767, ptr);

    ASSERT_STR2BUF_NULL(int16, "");
    ASSERT_STR2BUF_NULL(int16, "abc");
    ASSERT_STR2BUF_NULL(int16, "32768");
    ASSERT_STR2BUF_NULL(int16, "-32769");
}

void test_str2buf_uint16(void) {
    uint16_t *ptr;

    ASSERT_STR2BUF(uint16, "0", 0, ptr);
    ASSERT_STR2BUF(uint16, "0x0", 0, ptr);
    ASSERT_STR2BUF(uint16, "1", 1, ptr);
    ASSERT_STR2BUF(uint16, "65535", 65535, ptr);
    ASSERT_STR2BUF(uint16, "0xFFFF", 65535, ptr);

    ASSERT_STR2BUF_NULL(uint16, "");
    ASSERT_STR2BUF_NULL(uint16, "abc");
    ASSERT_STR2BUF_NULL(uint16, "65536");
    ASSERT_STR2BUF_NULL(uint16, "-1");
}

void test_str2buf_int32(void) {
    int32_t *ptr;

    ASSERT_STR2BUF(int32, "0", 0, ptr);
    ASSERT_STR2BUF(int32, "1", 1, ptr);
    ASSERT_STR2BUF(int32, "-1", -1, ptr);

    ASSERT_STR2BUF_NULL(int32, "");
    ASSERT_STR2BUF_NULL(int32, "abc");
}

void test_str2buf_uint32(void) {
    uint32_t *ptr;

    ASSERT_STR2BUF(uint32, "0", 0, ptr);
    ASSERT_STR2BUF(uint32, "0x0", 0, ptr);
    ASSERT_STR2BUF(uint32, "1", 1, ptr);
    ASSERT_STR2BUF(uint32, "123", 123, ptr);
    ASSERT_STR2BUF(uint32, "0x1FF", 0x1FF, ptr);

    ASSERT_STR2BUF_NULL(uint32, "");
    ASSERT_STR2BUF_NULL(uint32, "abc");
    ASSERT_STR2BUF_NULL(uint32, "-1");
}

void test_str2buf_int64(void) {
    int64_t *ptr;

    ASSERT_STR2BUF(int64, "0", 0, ptr);
    ASSERT_STR2BUF(int64, "1", 1, ptr);
    ASSERT_STR2BUF(int64, "-1", -1, ptr);

    ASSERT_STR2BUF_NULL(int64, "");
    ASSERT_STR2BUF_NULL(int64, "abc");
}

void test_str2buf_uint64(void) {
    uint64_t *ptr;

    ASSERT_STR2BUF(uint64, "0", 0, ptr);
    ASSERT_STR2BUF(uint64, "0x0", 0, ptr);
    ASSERT_STR2BUF(uint64, "1", 1, ptr);
    ASSERT_STR2BUF(uint64, "123", 123, ptr);
    ASSERT_STR2BUF(uint64, "0x1FF", 0x1FF, ptr);

    ASSERT_STR2BUF_NULL(uint64, "");
    ASSERT_STR2BUF_NULL(uint64, "abc");
    ASSERT_STR2BUF_NULL(uint64, "-1");
}

void test_str2buf_float32(void) {
    float *ptr;

    ASSERT_STR2BUF(float32, "0", 0.0, ptr);
    ASSERT_STR2BUF(float32, "1.0", 1.0, ptr);
    ASSERT_STR2BUF(float32, "1", 1.0, ptr);

    ASSERT_STR2BUF_NULL(float32, "");
    ASSERT_STR2BUF_NULL(float32, "abc");
}

void test_str2buf_float64(void) {
    double *ptr;

    ASSERT_STR2BUF(float64, "0", 0.0, ptr);
    ASSERT_STR2BUF(float64, "1.0", 1.0, ptr);
    ASSERT_STR2BUF(float64, "1", 1.0, ptr);

    ASSERT_STR2BUF_NULL(float64, "");
    ASSERT_STR2BUF_NULL(float64, "abc");
}


void test_str2buf_bytes(void) {
    uint8_t *buf = NULL;
    size_t buf_len = 0;

    buf = str2buf_bytes("00", &buf_len);
    assert(buf && (buf_len == 1) && (buf[0] == 0));
    free(buf);
    buf = str2buf_bytes("012345", &buf_len);
    assert(buf && (buf_len == 3) && (buf[0] == 0x1) && (buf[1] == 0x23) && buf[2] == 0x45);
    free(buf);

    buf = str2buf_bytes("", &buf_len);
    assert(buf == NULL);
    buf = str2buf_bytes("0", &buf_len);
    assert(buf == NULL);
    buf = str2buf_bytes("123", &buf_len);
    assert(buf == NULL);
}

int main(void) {
    test_str2buf_bool();
    test_str2buf_int8();
    test_str2buf_uint8();
    test_str2buf_int16();
    test_str2buf_uint16();
    test_str2buf_int32();
    test_str2buf_uint32();
    test_str2buf_int64();
    test_str2buf_uint64();
    test_str2buf_float32();
    test_str2buf_float64();
    test_str2buf_bytes();
    return 0;
}
