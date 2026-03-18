#include "display_driver.h"
#include "display.h"
#include "pio_qspi.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include <string.h>

#define LCD_X_OFFSET 6

/*
 * CO5300 QSPI command protocol:
 *   Command packet:  [0x02, 0x00, cmd, 0x00, ...data]   (sent in 1-bit mode)
 *   Bulk pixel data: [0x32, 0x00, 0x2C, 0x00, ...pixels] (header 1-bit, pixels 4-bit)
 */

static void cs_select(void)   { gpio_put(DISP_PIN_CS, 0); }
static void cs_deselect(void) { gpio_put(DISP_PIN_CS, 1); }

static void send_cmd(uint8_t cmd, const uint8_t *data, size_t len) {
    uint8_t header[4] = {0x02, 0x00, cmd, 0x00};
    cs_select();
    pio_qspi_1bit_write_blocking(header, 4);
    if (data && len > 0)
        pio_qspi_1bit_write_blocking(data, len);
    cs_deselect();
}

/* Static flush buffer: 4-byte header + pixel data (2 bytes per pixel) */
static uint8_t flush_buf[4 + DISPLAY_BUF_PIXELS * 2];

void display_init(void) {
    gpio_init(DISP_PIN_CS);
    gpio_set_dir(DISP_PIN_CS, GPIO_OUT);
    gpio_put(DISP_PIN_CS, 1);

    gpio_init(DISP_PIN_RST);
    gpio_set_dir(DISP_PIN_RST, GPIO_OUT);

    gpio_init(DISP_PIN_BL);
    gpio_set_dir(DISP_PIN_BL, GPIO_OUT);
    gpio_put(DISP_PIN_BL, 1);

    /* Hardware reset */
    gpio_put(DISP_PIN_RST, 1);
    sleep_ms(10);
    gpio_put(DISP_PIN_RST, 0);
    sleep_ms(10);
    gpio_put(DISP_PIN_RST, 1);
    sleep_ms(120);

    pio_qspi_init(DISP_PIN_SCK, DISP_PIN_D0, 75000000);

    /* CO5300 init sequence */
    send_cmd(0x11, NULL, 0);                        // Sleep out
    sleep_ms(120);
    uint8_t d;
    d = 0x80; send_cmd(0xC4, &d, 1);
    uint8_t d44[2] = {0x01, 0xD7}; send_cmd(0x44, d44, 2);
    d = 0x00; send_cmd(0x35, &d, 1);
    d = 0x20; send_cmd(0x53, &d, 1);
    sleep_ms(10);
    send_cmd(0x29, NULL, 0);                        // Display on
    sleep_ms(10);
    d = 0xA0; send_cmd(0x51, &d, 1);               // Brightness
    send_cmd(0x20, NULL, 0);                        // Inversion off
    d = 0x00; send_cmd(0x36, &d, 1);               // Memory access control
    d = 0x05; send_cmd(0x3A, &d, 1);               // Pixel format: RGB565
}

static void set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    uint16_t sx = x1 + LCD_X_OFFSET;
    uint16_t ex = x2 + LCD_X_OFFSET;
    uint8_t col[4] = {sx >> 8, sx & 0xFF, ex >> 8, ex & 0xFF};
    uint8_t row[4] = {y1 >> 8, y1 & 0xFF, y2 >> 8, y2 & 0xFF};
    send_cmd(0x2A, col, 4);
    send_cmd(0x2B, row, 4);
}

void display_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    uint16_t x1 = area->x1;
    uint16_t y1 = area->y1;
    uint16_t x2 = area->x2;
    uint16_t y2 = area->y2;

    set_window(x1, y1, x2, y2);

    uint32_t pixel_count = (uint32_t)(x2 - x1 + 1) * (y2 - y1 + 1);
    uint32_t data_len = pixel_count * 2;

    /* Bulk write header (sent in 1-bit mode) */
    flush_buf[0] = 0x32;
    flush_buf[1] = 0x00;
    flush_buf[2] = 0x2C;
    flush_buf[3] = 0x00;

    /* Byte-swap RGB565 pixels to big-endian */
    uint16_t *src = (uint16_t *)px_map;
    uint8_t *dst = flush_buf + 4;
    for (uint32_t i = 0; i < pixel_count; i++) {
        dst[i * 2]     = src[i] >> 8;
        dst[i * 2 + 1] = src[i] & 0xFF;
    }

    cs_select();
    pio_qspi_1bit_write_blocking(flush_buf, 4);
    pio_qspi_4bit_write_blocking(flush_buf + 4, data_len);
    cs_deselect();

    lv_display_flush_ready(disp);
}

void display_set_brightness(uint8_t level) {
    if (level > 100) level = 100;
    /* Map 0-100 → 0x25-0xFF */
    uint8_t val = 0x25 + (uint16_t)level * (0xFF - 0x25) / 100;
    send_cmd(0x51, &val, 1);
}

void display_test_fill(uint16_t color) {
    set_window(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);

    uint8_t header[4] = {0x32, 0x00, 0x2C, 0x00};
    cs_select();
    pio_qspi_1bit_write_blocking(header, 4);

    /* Fill in chunks to avoid stack overflow */
    #define FILL_CHUNK 256
    uint8_t chunk[FILL_CHUNK * 2];
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    for (int i = 0; i < FILL_CHUNK; i++) {
        chunk[i * 2]     = hi;
        chunk[i * 2 + 1] = lo;
    }

    uint32_t total = (uint32_t)DISPLAY_WIDTH * DISPLAY_HEIGHT;
    while (total > 0) {
        uint32_t n = total > FILL_CHUNK ? FILL_CHUNK : total;
        pio_qspi_4bit_write_blocking(chunk, n * 2);
        total -= n;
    }

    cs_deselect();
}
