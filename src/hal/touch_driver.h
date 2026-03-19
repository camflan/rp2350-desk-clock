#ifndef TOUCH_DRIVER_H
#define TOUCH_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Waveshare RP2350-Touch-AMOLED-1.43 touch pin assignments
 * (verified from board schematic V2.0)
 *
 * Touch IC: FT6236 family at I2C addr 0x38
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

typedef struct {
    uint16_t x;
    uint16_t y;
    bool     pressed;
} touch_event_t;

void touch_init(void);
void touch_read(touch_event_t *event);

#endif
