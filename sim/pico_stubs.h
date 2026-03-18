/**
 * Minimal stubs for pico/stdlib.h types used by shared source files.
 * Only used by the desktop simulator build.
 */
#ifndef PICO_STUBS_H
#define PICO_STUBS_H

#include <stdint.h>
#include <time.h>
#include <sys/time.h>

typedef uint64_t absolute_time_t;

static inline absolute_time_t get_absolute_time(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000ULL + tv.tv_usec;
}

static inline int64_t absolute_time_diff_us(absolute_time_t from, absolute_time_t to) {
    return (int64_t)(to - from);
}

static inline void sleep_ms(uint32_t ms) {
    struct timespec ts = {.tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1000000L};
    nanosleep(&ts, NULL);
}

#endif
