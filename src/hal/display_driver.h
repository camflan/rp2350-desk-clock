// src/hal/display_driver.h
#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "lvgl.h"

// Waveshare RP2350-Touch-LCD-1.43 pin definitions (QSPI mode)
// Source: https://github.com/jblanked/Waveshare/blob/master/RP2350-Touch-LCD-1.43/src/SDK/lcd/lcd.h
#define DISPLAY_SPI_PORT spi1  // Use SPI1
#define DISPLAY_PIN_SCK  10    // SCLK
#define DISPLAY_PIN_MOSI 11    // D0 (MOSI)
#define DISPLAY_PIN_CS   15    // Chip Select
#define DISPLAY_PIN_DC   8     // Data/Command (if needed for SPI mode)
#define DISPLAY_PIN_RST  16    // Reset
#define DISPLAY_PIN_BL   19    // Power/Backlight

// QSPI data lines (for QSPI mode)
#define DISPLAY_PIN_D0   11
#define DISPLAY_PIN_D1   12
#define DISPLAY_PIN_D2   13
#define DISPLAY_PIN_D3   14


#define DISPLAY_WIDTH  466
#define DISPLAY_HEIGHT 466

void display_init(void);
void display_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
void display_set_brightness(uint8_t level);  // 0-100
void display_test_fill(uint16_t color);      // Hardware test function

#endif
