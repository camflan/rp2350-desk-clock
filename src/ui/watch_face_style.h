#ifndef WATCH_FACE_STYLE_H
#define WATCH_FACE_STYLE_H

#include "lvgl.h"
#include "sweep.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    FACE_CLASSIC = 0,
    FACE_SPEEDMASTER = 1,
    FACE_FXD = 2,
    FACE_STYLE_COUNT
} watch_face_id_t;

typedef struct {
    const char *name;
    const char *short_name;  /* for settings buttons */

    /* Colors */
    lv_color_t bg;
    lv_color_t hand_color;
    lv_color_t second_color;
    lv_color_t marker_primary;   /* 12/3/6/9 */
    lv_color_t marker_secondary;
    lv_color_t date_color;

    /* Hand geometry (pixels from center) */
    int16_t hour_hand_len;
    int16_t minute_hand_len;
    int16_t second_hand_len;
    int16_t hour_hand_width;
    int16_t minute_hand_width;
    int16_t second_hand_width;

    /* Marker geometry */
    int16_t marker_outer;
    int16_t marker_inner;
    int16_t marker_width;
    int16_t marker_major_width;
    bool    marker_rounded;       /* rounded vs flat line caps */

    /* Minute track (60 small ticks between markers and bezel) */
    bool    show_minute_track;
    int16_t minute_track_outer;
    int16_t minute_track_inner;

    /* 12 o'clock triangle marker (replaces normal marker at 12) */
    bool    show_12_triangle;
    int16_t triangle_outer;
    int16_t triangle_inner;
    int16_t triangle_width;       /* base width of triangle */

    /* Features */
    bool         show_date;
    bool         show_bezel;
    int16_t      bezel_offset_deg;
    sweep_mode_t default_sweep;
} watch_face_style_t;

const watch_face_style_t *watch_face_get_style(watch_face_id_t id);

#endif
