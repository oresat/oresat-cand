/**
 * ECSS SpaceCraft Elapsed Time (SCET) and spacecraft Universal Time
 * Coordinated (UTC) time code formats.
 */

#ifndef _ECSS_TIME_H_
#define _ECSS_TIME_H_

#include <stdint.h>

typedef union {
    uint64_t raw;
    struct {
        uint32_t coarse : 32; // seconds
        uint32_t fine   : 24; // sub-seconds
    };
} ecss_scet_t;

typedef union {
    uint64_t raw;
    struct {
        uint16_t day : 16;
        uint32_t ms  : 32;
        uint16_t us  : 16;
    };
} ecss_utc_t;

void get_ecss_scet(ecss_scet_t *scet);
void set_ecss_scet(const ecss_scet_t *scet);
void get_ecss_utc(ecss_utc_t *utc);
void set_ecss_utc(const ecss_utc_t *utc);

#endif
