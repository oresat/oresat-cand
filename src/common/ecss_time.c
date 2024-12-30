#include <stddef.h>
#include <sys/time.h>
#include "ecss_time.h"

#define SECS_IN_DAY (24 * 60 * 60)

void get_ecss_scet(ecss_scet_t *scet) {
    if (!scet) {
        return;
    }

    struct timeval time;
    gettimeofday(&time, NULL);

    scet->coarse = time.tv_sec;
    scet->fine = time.tv_usec;
}

void set_ecss_scet(const ecss_scet_t *scet) {
    if (!scet) {
        return;
    }

    struct timeval time;
    time.tv_sec = scet->coarse;
    time.tv_usec = scet->fine;

    settimeofday(&time, NULL);
}

void get_ecss_utc(ecss_utc_t *utc) {
    if (!utc) {
        return;
    }

    struct timeval time;
    gettimeofday(&time, NULL);

    utc->day = time.tv_sec / SECS_IN_DAY;
    utc->ms = time.tv_usec / 1000;
    utc->us = time.tv_usec % 1000;
}

void set_ecss_utc(const ecss_utc_t *utc) {
    if (!utc) {
        return;
    }

    struct timeval time;
    time.tv_sec = utc->day * SECS_IN_DAY;
    time.tv_usec = utc->ms * 1000 + utc->us;

    settimeofday(&time, NULL);
}
