#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <stdint.h>
#include "lvgl.h"

/* Waveshare RP2350-Touch-AMOLED-1.43 pin assignments */
#define DISP_PIN_CS   15
#define DISP_PIN_DC    8
#define DISP_PIN_RST  16
#define DISP_PIN_BL   19
#define DISP_PIN_SCK  10
#define DISP_PIN_D0   11

void display_init(void);
void display_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
void display_set_brightness(uint8_t level);
void display_test_fill(uint16_t color);

#endif
