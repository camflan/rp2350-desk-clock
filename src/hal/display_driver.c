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

static void display_controller_init(void) {
    // CO5300 controller initialization sequence
    // Based on research from:
    // - https://github.com/kodediy/esp_lcd_co5300
    // - https://github.com/Infineon/display-amoled-co5300
    // - CO5300 datasheet references
    //
    // Note: This is a conservative init sequence for CO5300 in SPI mode.
    // The CO5300 supports SPI, QSPI, and MIPI-DSI interfaces.

    printf("Initializing CO5300 controller...\n");

    // Vendor-specific command enable sequence
    display_write_cmd(0xFE);  // Page selection
    uint8_t page = 0x00;
    display_write_data(&page, 1);

    display_write_cmd(0xEF);  // Command enable
    uint8_t enable = 0x00;
    display_write_data(&enable, 1);

    // Power control settings
    display_write_cmd(0x80);  // Power control
    uint8_t pwr = 0x11;
    display_write_data(&pwr, 1);

    display_write_cmd(0x81);  // Voltage setting
    uint8_t volt = 0x70;
    display_write_data(&volt, 1);

    // Sleep out - wake up display
    display_write_cmd(0x11);
    sleep_ms(120);  // Must wait 120ms after sleep out

    // Pixel format - RGB565 (16-bit)
    display_write_cmd(0x3A);
    uint8_t pixel_format = 0x55;  // 16-bit/pixel
    display_write_data(&pixel_format, 1);

    // Memory access control - rotation and mirroring
    display_write_cmd(0x36);
    uint8_t madctl = 0x00;  // Normal orientation
    display_write_data(&madctl, 1);

    // Column address set (0 to 465)
    display_write_cmd(0x2A);
    uint8_t col_addr[4] = {0x00, 0x00, 0x01, 0xD1};  // 0 to 465
    display_write_data(col_addr, 4);

    // Row address set (0 to 465)
    display_write_cmd(0x2B);
    uint8_t row_addr[4] = {0x00, 0x00, 0x01, 0xD1};  // 0 to 465
    display_write_data(row_addr, 4);

    // Brightness control
    display_write_cmd(0x51);  // Write display brightness
    uint8_t brightness = 0xFF;  // Maximum brightness
    display_write_data(&brightness, 1);

    // Display on
    display_write_cmd(0x29);
    sleep_ms(20);

    printf("CO5300 controller initialized\n");
}

void display_init(void) {
    printf("Initializing display...\n");

    display_spi_init();
    display_reset();
    display_controller_init();

    printf("Display initialized\n");
}

void display_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    // Calculate region dimensions
    uint16_t width = area->x2 - area->x1 + 1;
    uint16_t height = area->y2 - area->y1 + 1;
    size_t pixel_count = width * height;

    // Set column address window
    display_write_cmd(0x2A);
    uint8_t col_addr[4] = {
        (area->x1 >> 8) & 0xFF,  // Start column high byte
        area->x1 & 0xFF,         // Start column low byte
        (area->x2 >> 8) & 0xFF,  // End column high byte
        area->x2 & 0xFF          // End column low byte
    };
    display_write_data(col_addr, 4);

    // Set row address window
    display_write_cmd(0x2B);
    uint8_t row_addr[4] = {
        (area->y1 >> 8) & 0xFF,  // Start row high byte
        area->y1 & 0xFF,         // Start row low byte
        (area->y2 >> 8) & 0xFF,  // End row high byte
        area->y2 & 0xFF          // End row low byte
    };
    display_write_data(row_addr, 4);

    // Write pixel data to RAM
    display_write_cmd(0x2C);  // Memory write
    display_write_data(px_map, pixel_count * 2);  // 2 bytes per pixel (RGB565)

    // Notify LVGL that flushing is complete
    lv_display_flush_ready(disp);
}

void display_set_brightness(uint8_t level) {
    // TODO: Implement PWM backlight control
    printf("Set brightness: %d%%\n", level);
}
