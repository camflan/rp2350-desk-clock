// src/hal/display_driver.c
#include "display_driver.h"
#include "pio_qspi.h"
#include "hardware/gpio.h"
#include "pico/time.h"
#include <stdio.h>
#include <stdlib.h>

// Waveshare RP2350-Touch-AMOLED-1.43 Display Driver
// Based on jblanked/Waveshare implementation
//
// Display: 1.43" round AMOLED, 466x466 pixels
// Controller: CO5300 driver IC
// Interface: QSPI via PIO
//
// CO5300 Protocol:
// - Commands use header: [0x02, 0x00, cmd, 0x00] + data
// - Bulk pixel data uses header: [0x32, 0x00, 0x2C, 0x00] + pixels
// - No DC pin needed - protocol is embedded in data stream
// - Display requires X offset of 6 pixels

#define LCD_X_OFFSET 6

static void display_qspi_init(void) {
    printf("Initializing QSPI via PIO…\n");

    // Initialize RST and power pins
    gpio_init(DISPLAY_PIN_RST);
    gpio_set_dir(DISPLAY_PIN_RST, GPIO_OUT);

    gpio_init(DISPLAY_PIN_BL);
    gpio_set_dir(DISPLAY_PIN_BL, GPIO_OUT);
    gpio_put(DISPLAY_PIN_BL, 1);  // Turn on power
    printf("  Power pin GPIO%d enabled\n", DISPLAY_PIN_BL);

    // Initialize CS pin
    gpio_init(DISPLAY_PIN_CS);
    gpio_set_dir(DISPLAY_PIN_CS, GPIO_OUT);
    gpio_put(DISPLAY_PIN_CS, 1);  // CS high (inactive)
    printf("  CS pin GPIO%d initialized\n", DISPLAY_PIN_CS);

    // Initialize PIO QSPI - 75 MHz
    pio_qspi_init(DISPLAY_PIN_SCK, DISPLAY_PIN_D0, 75000000);

    printf("QSPI initialized\n");
}

static void display_reset(void) {
    gpio_put(DISPLAY_PIN_RST, 0);
    sleep_ms(100);
    gpio_put(DISPLAY_PIN_RST, 1);
    sleep_ms(200);
}

// Send command with data using CO5300 protocol
static void lcd_send_cmd_data(uint8_t cmd, const uint8_t *data, size_t data_len) {
    size_t total_len = 4 + data_len;
    uint8_t *buffer = malloc(total_len);
    if (buffer == NULL) return;

    // CO5300 protocol header
    buffer[0] = 0x02;  // Command indicator
    buffer[1] = 0x00;
    buffer[2] = cmd;   // Actual command
    buffer[3] = 0x00;

    // Copy data bytes
    for (size_t i = 0; i < data_len; i++) {
        buffer[4 + i] = data[i];
    }

    // Send with CS control
    gpio_put(DISPLAY_PIN_CS, 0);
    pio_qspi_1bit_write_blocking(buffer, total_len);
    gpio_put(DISPLAY_PIN_CS, 1);

    free(buffer);
}

static void display_controller_init(void) {
    // CO5300 controller initialization sequence
    // Based on jblanked/Waveshare working implementation
    printf("Initializing CO5300 controller…\n");

    // Sleep out
    lcd_send_cmd_data(0x11, NULL, 0);
    sleep_ms(120);  // Must wait 120ms after sleep out

    // Additional CO5300 specific commands
    uint8_t data;

    data = 0x80;
    lcd_send_cmd_data(0xC4, &data, 1);

    uint8_t data_44[2] = {0x01, 0xD7};
    lcd_send_cmd_data(0x44, data_44, 2);

    data = 0x00;
    lcd_send_cmd_data(0x35, &data, 1);

    data = 0x20;
    lcd_send_cmd_data(0x53, &data, 1);
    sleep_ms(10);

    // Display on
    lcd_send_cmd_data(0x29, NULL, 0);
    sleep_ms(10);

    // Brightness
    data = 0xA0;
    lcd_send_cmd_data(0x51, &data, 1);

    // Inversion off
    lcd_send_cmd_data(0x20, NULL, 0);

    // Memory access control - rotation and mirroring
    data = 0x00;
    lcd_send_cmd_data(0x36, &data, 1);

    // Pixel format - 16-bit RGB565
    data = 0x05;  // 16-bit/pixel
    lcd_send_cmd_data(0x3A, &data, 1);

    printf("CO5300 controller initialized\n");
}

void display_init(void) {
    printf("Initializing display…\n");

    display_qspi_init();
    display_reset();
    display_controller_init();

    printf("Display initialized\n");
}

void display_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    // Calculate region dimensions
    uint16_t width = area->x2 - area->x1 + 1;
    uint16_t height = area->y2 - area->y1 + 1;
    size_t pixel_count = width * height;

    // Apply X offset for display
    uint16_t x1 = area->x1 + LCD_X_OFFSET;
    uint16_t x2 = area->x2 + LCD_X_OFFSET;

    // Set column address window (with X offset)
    uint8_t col_addr[4] = {
        (x1 >> 8) & 0xFF,  // Start column high byte
        x1 & 0xFF,         // Start column low byte
        (x2 >> 8) & 0xFF,  // End column high byte
        x2 & 0xFF          // End column low byte
    };
    lcd_send_cmd_data(0x2A, col_addr, 4);

    // Set row address window
    uint8_t row_addr[4] = {
        (area->y1 >> 8) & 0xFF,  // Start row high byte
        area->y1 & 0xFF,         // Start row low byte
        (area->y2 >> 8) & 0xFF,  // End row high byte
        area->y2 & 0xFF          // End row low byte
    };
    lcd_send_cmd_data(0x2B, row_addr, 4);

    // Send pixel data using bulk transfer header (0x32)
    size_t total_len = 4 + (pixel_count * 2);
    uint8_t *buffer = malloc(total_len);
    if (buffer == NULL) {
        lv_display_flush_ready(disp);
        return;
    }

    // CO5300 bulk pixel transfer header
    buffer[0] = 0x32;  // Bulk transfer indicator
    buffer[1] = 0x00;
    buffer[2] = 0x2C;  // Memory write command
    buffer[3] = 0x00;

    // Copy pixel data and byte-swap for big-endian
    for (size_t i = 0; i < pixel_count; i++) {
        uint16_t color = ((uint16_t *)px_map)[i];
        buffer[4 + i * 2] = color >> 8;      // High byte
        buffer[4 + i * 2 + 1] = color & 0xFF; // Low byte
    }

    // Send entire frame with CS control
    gpio_put(DISPLAY_PIN_CS, 0);
    pio_qspi_1bit_write_blocking(buffer, 4);  // Send header in 1-bit mode
    pio_qspi_4bit_write_blocking(buffer + 4, pixel_count * 2);  // Send pixels in 4-bit mode
    gpio_put(DISPLAY_PIN_CS, 1);

    free(buffer);

    // Notify LVGL that flushing is complete
    lv_display_flush_ready(disp);
}

void display_set_brightness(uint8_t level) {
    if (level > 100) level = 100;

    // Convert 0-100 to OLED brightness range (0x25 to 0xFF)
    uint8_t oled_brightness = 0x25 + (level * (0xFF - 0x25)) / 100;
    lcd_send_cmd_data(0x51, &oled_brightness, 1);

    printf("Set brightness: %d%% (0x%02X)\n", level, oled_brightness);
}

// Hardware test: Fill screen with solid color
void display_test_fill(uint16_t color) {
    printf("Hardware test: Filling screen with color 0x%04X\n", color);

    // Set full screen window (with X offset: 6 to 471)
    uint16_t x_start = LCD_X_OFFSET;
    uint16_t x_end = 466 - 1 + LCD_X_OFFSET;  // 465 + 6 = 471
    uint8_t col_addr[4] = {
        (x_start >> 8) & 0xFF,
        x_start & 0xFF,
        (x_end >> 8) & 0xFF,
        x_end & 0xFF
    };
    lcd_send_cmd_data(0x2A, col_addr, 4);

    uint8_t row_addr[4] = {0x00, 0x00, 0x01, 0xD1};  // 0 to 465
    lcd_send_cmd_data(0x2B, row_addr, 4);

    // Prepare pixel data - send in chunks to avoid huge malloc
    #define CHUNK_SIZE 1024
    uint8_t buffer[4 + CHUNK_SIZE * 2];

    // Header for bulk transfer
    buffer[0] = 0x32;
    buffer[1] = 0x00;
    buffer[2] = 0x2C;
    buffer[3] = 0x00;

    // Fill buffer with color (byte-swapped)
    for (int i = 0; i < CHUNK_SIZE; i++) {
        buffer[4 + i * 2] = color >> 8;
        buffer[4 + i * 2 + 1] = color & 0xFF;
    }

    // Send data in chunks (466*466 = 217,156 pixels)
    uint32_t total_pixels = 466 * 466;
    uint32_t pixels_sent = 0;

    gpio_put(DISPLAY_PIN_CS, 0);
    pio_qspi_1bit_write_blocking(buffer, 4);  // Send header once

    while (pixels_sent < total_pixels) {
        uint32_t pixels_to_send = (total_pixels - pixels_sent > CHUNK_SIZE) ? CHUNK_SIZE : (total_pixels - pixels_sent);
        pio_qspi_4bit_write_blocking(buffer + 4, pixels_to_send * 2);
        pixels_sent += pixels_to_send;
    }

    gpio_put(DISPLAY_PIN_CS, 1);

    printf("Hardware test complete: %d pixels sent\n", pixels_sent);
}
