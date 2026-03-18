/*
 * Touch driver for Waveshare RP2350-Touch-AMOLED-1.43
 *
 * IC: FT6146 (register-compatible with CST816S family)
 * I2C address: 0x15
 * Pin assignments verified from board schematic V2.0:
 *   SDA=GPIO6  SCL=GPIO7  INT=GPIO21  RST=GPIO22
 *
 * Register map (single-touch):
 *   0x01  Gesture ID
 *   0x02  Finger count (0 or 1)
 *   0x03  X position [11:8]
 *   0x04  X position [7:0]
 *   0x05  Y position [11:8]
 *   0x06  Y position [7:0]
 */

#include "touch_driver.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"

#define REG_GESTURE   0x01
#define REG_FINGERS   0x02
#define REG_X_HI      0x03
#define REG_X_LO      0x04
#define REG_Y_HI      0x05
#define REG_Y_LO      0x06

/* CST816S / FT6146 gesture IDs */
#define RAW_GESTURE_NONE        0x00
#define RAW_GESTURE_SWIPE_UP    0x01
#define RAW_GESTURE_SWIPE_DOWN  0x02
#define RAW_GESTURE_SWIPE_LEFT  0x03
#define RAW_GESTURE_SWIPE_RIGHT 0x04
#define RAW_GESTURE_TAP         0x05
#define RAW_GESTURE_DOUBLE_TAP  0x0B
#define RAW_GESTURE_LONG_PRESS  0x0C

static bool read_regs(uint8_t reg, uint8_t *buf, size_t len) {
    int ret = i2c_write_blocking(TOUCH_I2C_INST, TOUCH_I2C_ADDR, &reg, 1, true);
    if (ret < 0) return false;
    ret = i2c_read_blocking(TOUCH_I2C_INST, TOUCH_I2C_ADDR, buf, len, false);
    return ret >= 0;
}

static touch_gesture_t map_gesture(uint8_t raw) {
    switch (raw) {
    case RAW_GESTURE_SWIPE_LEFT:  return GESTURE_SWIPE_LEFT;
    case RAW_GESTURE_SWIPE_RIGHT: return GESTURE_SWIPE_RIGHT;
    case RAW_GESTURE_TAP:
    case RAW_GESTURE_DOUBLE_TAP:
    case RAW_GESTURE_LONG_PRESS:  return GESTURE_TAP;
    default:                      return GESTURE_NONE;
    }
}

void touch_init(void) {
    i2c_init(TOUCH_I2C_INST, 400 * 1000);

    gpio_set_function(TOUCH_PIN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(TOUCH_PIN_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(TOUCH_PIN_SDA);
    gpio_pull_up(TOUCH_PIN_SCL);

    /* Reset pulse */
    gpio_init(TOUCH_PIN_RST);
    gpio_set_dir(TOUCH_PIN_RST, GPIO_OUT);
    gpio_put(TOUCH_PIN_RST, 0);
    sleep_ms(10);
    gpio_put(TOUCH_PIN_RST, 1);
    sleep_ms(50);

    /* INT as input (active low, directly polled) */
    gpio_init(TOUCH_PIN_INT);
    gpio_set_dir(TOUCH_PIN_INT, GPIO_IN);
    gpio_pull_up(TOUCH_PIN_INT);
}

void touch_read(touch_event_t *event) {
    event->x       = 0;
    event->y       = 0;
    event->pressed = false;
    event->gesture = GESTURE_NONE;

    uint8_t buf[6];
    if (!read_regs(REG_GESTURE, buf, 6))
        return;

    /* buf[0]=gesture  buf[1]=fingers  buf[2..5]=x_hi,x_lo,y_hi,y_lo */
    uint8_t fingers = buf[1];
    if (fingers == 0)
        return;

    event->pressed = true;
    event->x       = ((uint16_t)(buf[2] & 0x0F) << 8) | buf[3];
    event->y       = ((uint16_t)(buf[4] & 0x0F) << 8) | buf[5];
    event->gesture = map_gesture(buf[0]);
}
