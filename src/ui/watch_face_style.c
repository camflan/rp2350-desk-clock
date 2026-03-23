#include "watch_face_style.h"

/*
 * ┌─────────────────────────────────────────────────────┐
 * │  Classic      Speedmaster         FXD               │
 * │  ────────     ──────────────      ──────────────    │
 * │  Simple       Omega-inspired      Tudor Pelagos     │
 * │  baton        wider batons,       FXD, navy dial,   │
 * │  markers,     no date, warm       wide hour hand,   │
 * │  red second   lume tones          60-min bezel      │
 * │  hand, date                                         │
 * └─────────────────────────────────────────────────────┘
 */

static const watch_face_style_t styles[FACE_STYLE_COUNT] = {
    [FACE_CLASSIC] = {
        .name       = "Classic",
        .short_name = "Classic",

        .bg               = {.red = 0x00, .green = 0x00, .blue = 0x00},
        .hand_color       = {.red = 0xE8, .green = 0xE8, .blue = 0xE8},
        .second_color     = {.red = 0xFF, .green = 0x40, .blue = 0x40},
        .marker_primary   = {.red = 0xE8, .green = 0xE8, .blue = 0xE8},
        .marker_secondary = {.red = 0x6B, .green = 0x6B, .blue = 0x6B},
        .date_color       = {.red = 0x6B, .green = 0x6B, .blue = 0x6B},

        .hour_hand_len   = 100,
        .minute_hand_len = 150,
        .second_hand_len = 170,
        .hour_hand_width   = 6,
        .minute_hand_width = 4,
        .second_hand_width = 2,

        .marker_outer      = 220,
        .marker_inner      = 200,
        .marker_width      = 2,
        .marker_major_width = 4,

        .show_date        = true,
        .show_bezel       = false,
        .bezel_offset_deg = 0,
        .default_sweep    = SWEEP_1HZ,
    },

    [FACE_SPEEDMASTER] = {
        .name       = "Speedmaster",
        .short_name = "Speedy",

        .bg               = {.red = 0x0A, .green = 0x0A, .blue = 0x0A},
        .hand_color       = {.red = 0xE8, .green = 0xE8, .blue = 0xD0},
        .second_color     = {.red = 0xE8, .green = 0xE8, .blue = 0xD0},
        .marker_primary   = {.red = 0xE8, .green = 0xE8, .blue = 0xD0},
        .marker_secondary = {.red = 0x88, .green = 0x88, .blue = 0x78},
        .date_color       = {.red = 0x88, .green = 0x88, .blue = 0x78},

        .hour_hand_len   = 95,
        .minute_hand_len = 145,
        .second_hand_len = 165,
        .hour_hand_width   = 8,
        .minute_hand_width = 5,
        .second_hand_width = 2,

        .marker_outer      = 218,
        .marker_inner      = 195,
        .marker_width      = 2,
        .marker_major_width = 5,

        .show_date        = false,
        .show_bezel       = false,
        .bezel_offset_deg = 0,
        .default_sweep    = SWEEP_3HZ,
    },

    [FACE_FXD] = {
        .name       = "FXD",
        .short_name = "FXD",

        .bg               = {.red = 0x0A, .green = 0x14, .blue = 0x28},
        .hand_color       = {.red = 0xE8, .green = 0xE8, .blue = 0xD8},
        .second_color     = {.red = 0xE8, .green = 0xE8, .blue = 0xD8},
        .marker_primary   = {.red = 0xE8, .green = 0xE8, .blue = 0xD0},
        .marker_secondary = {.red = 0x78, .green = 0x90, .blue = 0xA0},
        .date_color       = {.red = 0x78, .green = 0x90, .blue = 0xA0},

        .hour_hand_len   = 90,
        .minute_hand_len = 148,
        .second_hand_len = 168,
        .hour_hand_width   = 10,
        .minute_hand_width = 5,
        .second_hand_width = 2,

        .marker_outer      = 205,
        .marker_inner      = 185,
        .marker_width      = 3,
        .marker_major_width = 6,

        .show_date        = false,
        .show_bezel       = true,
        .bezel_offset_deg = 0,
        .default_sweep    = SWEEP_4HZ,
    },
};

const watch_face_style_t *watch_face_get_style(watch_face_id_t id) {
    if (id >= FACE_STYLE_COUNT) id = FACE_CLASSIC;
    return &styles[id];
}
