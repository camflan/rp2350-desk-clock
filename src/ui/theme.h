#ifndef THEME_H
#define THEME_H

#include "lvgl.h"
#include <stdint.h>

#define THEME_DARK  0
#define THEME_LIGHT 1
#define THEME_BLUE  2
#define THEME_RED   3
#define THEME_GREEN 4
#define THEME_COUNT 5

#define THEME_MAX_CALLBACKS 4

typedef struct {
    lv_color_t bg;
    lv_color_t primary;
    lv_color_t secondary;
    lv_color_t accent;
} theme_colors_t;

typedef void (*theme_cb_t)(const theme_colors_t *colors);

void                  theme_init(void);
void                  theme_apply(uint8_t id);
uint8_t               theme_get_current_id(void);
const theme_colors_t *theme_get_colors(void);

void theme_register_callback(theme_cb_t cb);
void theme_unregister_callback(theme_cb_t cb);

#endif
