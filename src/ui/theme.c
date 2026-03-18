#include "theme.h"
#include <stddef.h>

/*
 * Luxury watch-inspired color palettes.
 * All backgrounds near-black for AMOLED power efficiency.
 *
 * bg        = screen background
 * primary   = time text, clock hands, major markers
 * secondary = date text, minor markers, separator lines
 * accent    = second hand, active UI highlights
 */
static const theme_colors_t themes[THEME_COUNT] = {
    [THEME_DARK] = {
        .bg        = {.red = 0x00, .green = 0x00, .blue = 0x00},
        .primary   = {.red = 0xE8, .green = 0xE8, .blue = 0xE8},
        .secondary = {.red = 0x6B, .green = 0x6B, .blue = 0x6B},
        .accent    = {.red = 0xFF, .green = 0x40, .blue = 0x40},
    },
    [THEME_LIGHT] = {
        .bg        = {.red = 0x1A, .green = 0x1A, .blue = 0x1A},
        .primary   = {.red = 0xF5, .green = 0xF0, .blue = 0xE8},
        .secondary = {.red = 0xA0, .green = 0x98, .blue = 0x80},
        .accent    = {.red = 0xC8, .green = 0xA0, .blue = 0x50},
    },
    [THEME_BLUE] = {
        .bg        = {.red = 0x06, .green = 0x0C, .blue = 0x18},
        .primary   = {.red = 0xD0, .green = 0xD8, .blue = 0xE8},
        .secondary = {.red = 0x4A, .green = 0x5A, .blue = 0x78},
        .accent    = {.red = 0x4A, .green = 0x90, .blue = 0xD0},
    },
    [THEME_RED] = {
        .bg        = {.red = 0x10, .green = 0x08, .blue = 0x08},
        .primary   = {.red = 0xE8, .green = 0xD8, .blue = 0xD0},
        .secondary = {.red = 0x78, .green = 0x40, .blue = 0x40},
        .accent    = {.red = 0xD0, .green = 0x40, .blue = 0x40},
    },
    [THEME_GREEN] = {
        .bg        = {.red = 0x06, .green = 0x10, .blue = 0x08},
        .primary   = {.red = 0xD0, .green = 0xE0, .blue = 0xD0},
        .secondary = {.red = 0x40, .green = 0x68, .blue = 0x40},
        .accent    = {.red = 0xC8, .green = 0xA0, .blue = 0x50},
    },
};

static uint8_t current_id;
static theme_cb_t callbacks[THEME_MAX_CALLBACKS];

void theme_init(void) {
    current_id = THEME_DARK;
}

void theme_apply(uint8_t id) {
    if (id >= THEME_COUNT)
        return;

    current_id = id;
    const theme_colors_t *c = &themes[id];

    /* Update the active screen background */
    lv_obj_t *scr = lv_screen_active();
    if (scr) {
        lv_obj_set_style_bg_color(scr, c->bg, 0);
        lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    }

    for (int i = 0; i < THEME_MAX_CALLBACKS; i++) {
        if (callbacks[i])
            callbacks[i](c);
    }
}

uint8_t theme_get_current_id(void) {
    return current_id;
}

const theme_colors_t *theme_get_colors(void) {
    return &themes[current_id];
}

void theme_register_callback(theme_cb_t cb) {
    for (int i = 0; i < THEME_MAX_CALLBACKS; i++) {
        if (!callbacks[i]) {
            callbacks[i] = cb;
            return;
        }
    }
}

void theme_unregister_callback(theme_cb_t cb) {
    for (int i = 0; i < THEME_MAX_CALLBACKS; i++) {
        if (callbacks[i] == cb) {
            callbacks[i] = NULL;
            return;
        }
    }
}
