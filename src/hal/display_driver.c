// src/hal/display_driver.c
#include "display_driver.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "pico/time.h"
#include <stdio.h>

// Waveshare RP2350-Touch-AMOLED-1.43 Display Driver
//
// Research findings:
// - Board: Waveshare RP2350-Touch-AMOLED-1.43
// - Display: 1.43" round AMOLED, 466x466 pixels
// - Interface: SPI (display), I2C (touch - FT6146)
// - Display Controller: CO5300 driver IC (per Waveshare wiki)
// - Touch Controller: FT6146 (I2C interface)
// - Additional sensors: QMI8658 (IMU), PCF85063A (RTC)
// - Resources: https://www.waveshare.com/wiki/RP2350-Touch-AMOLED-1.43
//
// Note: Pin definitions are placeholders and need verification from:
// - Waveshare schematic: https://files.waveshare.com/wiki/RP2350-Touch-AMOLED-1.43/RP2350-Touch-AMOLED-1.43-Schematic.pdf
// - Waveshare demo code (C/C++, MicroPython, Arduino available)
//
// TODO: Update controller-specific init sequence from CO5300 datasheet

static void display_spi_init(void) {
    // Initialize SPI at 62.5 MHz
    spi_init(DISPLAY_SPI_PORT, 62500000);

    gpio_set_function(DISPLAY_PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(DISPLAY_PIN_MOSI, GPIO_FUNC_SPI);

    // CS, DC, RST as outputs
    gpio_init(DISPLAY_PIN_CS);
    gpio_set_dir(DISPLAY_PIN_CS, GPIO_OUT);
    gpio_put(DISPLAY_PIN_CS, 1);

    gpio_init(DISPLAY_PIN_DC);
    gpio_set_dir(DISPLAY_PIN_DC, GPIO_OUT);

    gpio_init(DISPLAY_PIN_RST);
    gpio_set_dir(DISPLAY_PIN_RST, GPIO_OUT);
}

static void display_reset(void) {
    gpio_put(DISPLAY_PIN_RST, 1);
    sleep_ms(5);
    gpio_put(DISPLAY_PIN_RST, 0);
    sleep_ms(20);
    gpio_put(DISPLAY_PIN_RST, 1);
    sleep_ms(150);
}

static void display_write_cmd(uint8_t cmd) {
    gpio_put(DISPLAY_PIN_CS, 0);
    gpio_put(DISPLAY_PIN_DC, 0);  // Command
    spi_write_blocking(DISPLAY_SPI_PORT, &cmd, 1);
    gpio_put(DISPLAY_PIN_CS, 1);
}

static void display_write_data(const uint8_t *data, size_t len) {
    gpio_put(DISPLAY_PIN_CS, 0);
    gpio_put(DISPLAY_PIN_DC, 1);  // Data
    spi_write_blocking(DISPLAY_SPI_PORT, data, len);
    gpio_put(DISPLAY_PIN_CS, 1);
}

void display_init(void) {
    printf("Initializing display...\n");

    display_spi_init();
    display_reset();

    // TODO: Add controller-specific initialization sequence
    // This will come from Waveshare example code or datasheet

    printf("Display initialized\n");
}

void display_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    // TODO: Implement pixel transfer to display
    // For now, just acknowledge
    lv_display_flush_ready(disp);
}

void display_set_brightness(uint8_t level) {
    // TODO: Implement PWM backlight control
    printf("Set brightness: %d%%\n", level);
}
