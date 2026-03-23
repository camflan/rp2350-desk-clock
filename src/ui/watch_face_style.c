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

        .marker_outer       = 220,
        .marker_inner       = 200,
        .marker_width       = 2,
        .marker_major_width = 4,
        .marker_rounded     = true,

        .show_minute_track = false,
        .show_12_triangle  = false,

        .show_date        = true,
        .show_bezel       = false,
        .bezel_offset_deg = 0,
        .default_sweep    = SWEEP_1HZ,
    },

    /*
     * Omega Speedmaster Professional (Moonwatch)
     *
     * Pure black dial, bright white/silver hands and markers.
     * Thin elongated baton markers, uniform width. Fine minute
     * track around dial edge. No date, no bezel.
     */
    [FACE_SPEEDMASTER] = {
        .name       = "Speedmaster",
        .short_name = "Speedy",

        .bg               = {.red = 0x00, .green = 0x00, .blue = 0x00},
        .hand_color       = {.red = 0xF0, .green = 0xF0, .blue = 0xF0},
        .second_color     = {.red = 0xF0, .green = 0xF0, .blue = 0xF0},
        .marker_primary   = {.red = 0xF0, .green = 0xF0, .blue = 0xF0},
        .marker_secondary = {.red = 0x60, .green = 0x60, .blue = 0x60},
        .date_color       = {.red = 0x60, .green = 0x60, .blue = 0x60},

        .hour_hand_len   = 100,
        .minute_hand_len = 155,
        .second_hand_len = 175,
        .hour_hand_width   = 6,
        .minute_hand_width = 4,
        .second_hand_width = 2,

        .marker_outer       = 215,
        .marker_inner       = 190,
        .marker_width       = 3,
        .marker_major_width = 3,     /* uniform width — all batons equal */
        .marker_rounded     = false,

        .show_minute_track  = true,
        .minute_track_outer = 220,
        .minute_track_inner = 216,

        .show_12_triangle  = false,

        .show_date        = false,
        .show_bezel       = false,
        .bezel_offset_deg = 0,
        .default_sweep    = SWEEP_3HZ,
    },

    /*
     * Tudor Pelagos FXD
     *
     *   ┌──────────────────────────────────┐
     *   │  ▽  Bezel: 60 ticks, △ at 12    │
     *   │ ┌────────────────────────────┐   │
     *   │ │ ··· minute track (60 dots) │   │
     *   │ │  ▽  triangle at 12         │   │
     *   │ │  ▮  rectangular markers    │   │
     *   │ │     at 3, 4, 5, 7, 8, etc  │   │
     *   │ │  ▮▮ wider at 3, 6, 9       │   │
     *   │ │                            │   │
     *   │ │     hands (wide hour)      │   │
     *   │ └────────────────────────────┘   │
     *   └──────────────────────────────────┘
     */
    [FACE_FXD] = {
        .name       = "FXD",
        .short_name = "FXD",

        .bg               = {.red = 0x0C, .green = 0x18, .blue = 0x30},
        .hand_color       = {.red = 0xE8, .green = 0xE8, .blue = 0xD8},
        .second_color     = {.red = 0xC0, .green = 0xC0, .blue = 0xB8},
        .marker_primary   = {.red = 0xF0, .green = 0xF0, .blue = 0xE0},
        .marker_secondary = {.red = 0x50, .green = 0x68, .blue = 0x80},
        .date_color       = {.red = 0x50, .green = 0x68, .blue = 0x80},

        .hour_hand_len   = 90,
        .minute_hand_len = 150,
        .second_hand_len = 170,
        .hour_hand_width   = 12,
        .minute_hand_width = 6,
        .second_hand_width = 2,

        .marker_outer       = 195,
        .marker_inner       = 175,
        .marker_width       = 8,      /* rectangular blocks */
        .marker_major_width = 12,
        .marker_rounded     = false,   /* flat caps for block look */

        .show_minute_track  = true,
        .minute_track_outer = 200,
        .minute_track_inner = 196,

        .show_12_triangle   = true,
        .triangle_outer     = 195,
        .triangle_inner     = 170,
        .triangle_width     = 14,

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
