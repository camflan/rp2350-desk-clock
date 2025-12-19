// src/hal/display_driver.h
#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "lvgl.h"

// Waveshare RP2350-Touch-AMOLED-1.43 pin definitions
// TODO: Verify these from Waveshare schematic
#define DISPLAY_SPI_PORT spi1
#define DISPLAY_PIN_SCK  10
#define DISPLAY_PIN_MOSI 11
#define DISPLAY_PIN_CS   9
#define DISPLAY_PIN_DC   8
#define DISPLAY_PIN_RST  12
#define DISPLAY_PIN_BL   25

#define DISPLAY_WIDTH  466
#define DISPLAY_HEIGHT 466

void display_init(void);
void display_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
void display_set_brightness(uint8_t level);  // 0-100

#endif
