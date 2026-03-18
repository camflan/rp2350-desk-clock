#ifndef SWEEP_H
#define SWEEP_H

#include <stdint.h>

typedef enum {
    SWEEP_1HZ = 0,    /* Quartz tick — 1 beat/sec */
    SWEEP_3HZ = 1,    /* 21,600 vph — 3 beats/sec */
    SWEEP_4HZ = 2,    /* 28,800 vph — 4 beats/sec */
    SWEEP_5HZ = 3,    /* 36,000 vph — 5 beats/sec */
    SWEEP_SMOOTH = 4,  /* Spring Drive — continuous */
    SWEEP_MODE_COUNT
} sweep_mode_t;

/* Timer interval in ms for the given mode */
uint32_t sweep_interval_ms(sweep_mode_t mode);

/* Second hand angle for the given mode.
 * second: 0-59
 * elapsed_ms_in_second: 0-999
 * Returns angle in degrees (0-360) */
double sweep_second_angle(sweep_mode_t mode, uint8_t second,
                          uint32_t elapsed_ms_in_second);

#endif
