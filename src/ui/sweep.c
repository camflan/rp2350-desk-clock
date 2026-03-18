#include "sweep.h"

/*
 * Timer intervals per mode:
 *   1Hz  = 1000ms  (quartz tick)
 *   3Hz  =  333ms  (21,600 vph)
 *   4Hz  =  250ms  (28,800 vph)
 *   5Hz  =  200ms  (36,000 vph)
 *   Smooth = 20ms  (~50Hz, Spring Drive style)
 */
static const uint32_t intervals[SWEEP_MODE_COUNT] = {
    1000, 333, 250, 200, 20
};

/* Beats per second for quantized modes */
static const uint32_t beats_per_sec[SWEEP_MODE_COUNT] = {
    1, 3, 4, 5, 0
};

uint32_t sweep_interval_ms(sweep_mode_t mode) {
    if (mode >= SWEEP_MODE_COUNT)
        return intervals[SWEEP_1HZ];
    return intervals[mode];
}

/*
 * Quantized modes (1/3/4/5 Hz):
 *   Snap elapsed_ms to the nearest beat boundary, then compute
 *   base angle + fractional degrees for that beat position.
 *
 *     degrees_per_beat = 6.0 / beats_per_sec
 *     beat_index       = elapsed_ms / (1000 / beats_per_sec)
 *     angle            = second * 6.0 + beat_index * degrees_per_beat
 *
 * Smooth mode:
 *   Linear interpolation across the full second.
 *     angle = second * 6.0 + (elapsed_ms / 1000.0) * 6.0
 */
double sweep_second_angle(sweep_mode_t mode, uint8_t second,
                          uint32_t elapsed_ms_in_second) {
    double base = second * 6.0;

    if (mode >= SWEEP_MODE_COUNT)
        mode = SWEEP_1HZ;

    if (mode == SWEEP_SMOOTH) {
        return base + (elapsed_ms_in_second / 1000.0) * 6.0;
    }

    uint32_t bps = beats_per_sec[mode];
    uint32_t ms_per_beat = 1000 / bps;
    uint32_t beat_index = elapsed_ms_in_second / ms_per_beat;
    if (beat_index >= bps)
        beat_index = bps - 1;

    double degrees_per_beat = 6.0 / bps;
    return base + beat_index * degrees_per_beat;
}
