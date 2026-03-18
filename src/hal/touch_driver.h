#ifndef TOUCH_DRIVER_H
#define TOUCH_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Waveshare RP2350-Touch-AMOLED-1.43 touch pin assignments
 * (verified from board schematic V2.0)
 *
 * Touch IC: FT6146 (CST816S-register-compatible) at I2C addr 0x15
 * Bus: i2c1 @ 400 kHz
 *
 *   SDA  -> GPIO 6
 *   SCL  -> GPIO 7
 *   INT  -> GPIO 21
 *   RST  -> GPIO 22
 */
#define TOUCH_PIN_SDA  6
#define TOUCH_PIN_SCL  7
#define TOUCH_PIN_INT  21
#define TOUCH_PIN_RST  22

#define TOUCH_I2C_INST i2c1
#define TOUCH_I2C_ADDR 0x15

typedef enum {
    GESTURE_NONE       = 0,
    GESTURE_SWIPE_LEFT = 1,
    GESTURE_SWIPE_RIGHT = 2,
    GESTURE_TAP        = 3,
} touch_gesture_t;

typedef struct {
    uint16_t        x;
    uint16_t        y;
    bool            pressed;
    touch_gesture_t gesture;
} touch_event_t;

void touch_init(void);
void touch_read(touch_event_t *event);

#endif
