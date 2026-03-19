/*
 * Touch driver for Waveshare RP2350-Touch-AMOLED-1.43
 *
 * Pin assignments from board schematic V2.0:
 *   SDA=GPIO6  SCL=GPIO7  INT=GPIO21  RST=GPIO22
 *   Bus: i2c1 @ 400kHz
 *
 * The touch IC (CST816S family) enters deep sleep quickly after
 * reset. It only wakes on touch events. We must try multiple
 * candidate addresses and configure it in the brief post-reset
 * window or on first touch.
 */

#include "touch_driver.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include <stdio.h>

#define REG_GESTURE     0x01
#define REG_FINGERS     0x02
#define REG_X_HI        0x03
#define REG_X_LO        0x04
#define REG_Y_HI        0x05
#define REG_Y_LO        0x06
#define REG_CHIP_ID     0xA7
#define REG_AUTO_SLEEP  0xFE
#define REG_IRQ_CTL     0xFA
#define REG_MOTION_MASK 0xEC

/* CST816S gesture IDs */
#define RAW_GESTURE_NONE        0x00
#define RAW_GESTURE_SWIPE_UP    0x01
#define RAW_GESTURE_SWIPE_DOWN  0x02
#define RAW_GESTURE_SWIPE_LEFT  0x03
#define RAW_GESTURE_SWIPE_RIGHT 0x04
#define RAW_GESTURE_TAP         0x05
#define RAW_GESTURE_DOUBLE_TAP  0x0B
#define RAW_GESTURE_LONG_PRESS  0x0C

/* Candidate I2C addresses for CST816S / FT6146 family */
static const uint8_t candidate_addrs[] = {0x15, 0x5A, 0x38, 0x51};
#define NUM_CANDIDATES (sizeof(candidate_addrs) / sizeof(candidate_addrs[0]))

static uint8_t active_addr = 0;  /* 0 = not yet discovered */
static bool configured = false;

static bool try_read(uint8_t addr, uint8_t reg, uint8_t *buf, size_t len) {
    int ret = i2c_write_blocking(TOUCH_I2C_INST, addr, &reg, 1, true);
    if (ret < 0) return false;
    ret = i2c_read_blocking(TOUCH_I2C_INST, addr, buf, len, false);
    return ret >= 0;
}

static void try_write(uint8_t addr, uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    i2c_write_blocking(TOUCH_I2C_INST, addr, buf, 2, false);
}

static void configure_ic(uint8_t addr) {
    try_write(addr, REG_AUTO_SLEEP, 0x01);   /* disable auto-sleep */
    try_write(addr, REG_IRQ_CTL, 0x71);       /* enable touch + gesture IRQ */
    try_write(addr, REG_MOTION_MASK, 0x07);   /* all gesture types */
    configured = true;
    printf("[touch] configured addr=0x%02X\n", addr);
}

/*
 * Reset the touch IC and immediately try to find it before it
 * enters deep sleep (~5-10ms window after reset).
 */
static void reset_and_probe(void) {
    gpio_put(TOUCH_PIN_RST, 0);
    sleep_ms(5);
    gpio_put(TOUCH_PIN_RST, 1);
    sleep_ms(5);  /* must be fast — IC sleeps within ~10ms */

    for (unsigned i = 0; i < NUM_CANDIDATES; i++) {
        uint8_t chip_id = 0;
        if (try_read(candidate_addrs[i], REG_CHIP_ID, &chip_id, 1)) {
            printf("[touch] found IC at 0x%02X, chip_id=0x%02X\n",
                   candidate_addrs[i], chip_id);
            active_addr = candidate_addrs[i];
            configure_ic(active_addr);
            return;
        }
    }
    printf("[touch] no IC found after reset (will retry on touch)\n");
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

    gpio_init(TOUCH_PIN_RST);
    gpio_set_dir(TOUCH_PIN_RST, GPIO_OUT);

    gpio_init(TOUCH_PIN_INT);
    gpio_set_dir(TOUCH_PIN_INT, GPIO_IN);
    gpio_pull_up(TOUCH_PIN_INT);

    reset_and_probe();
}

void touch_read(touch_event_t *event) {
    event->x       = 0;
    event->y       = 0;
    event->pressed = false;
    event->gesture = GESTURE_NONE;

    static uint32_t poll_count = 0;
    poll_count++;

    if (poll_count % 5000 == 0)
        printf("[touch] heartbeat poll=%u INT=%d addr=0x%02X\n",
               poll_count, gpio_get(TOUCH_PIN_INT), active_addr);

    /*
     * If we haven't found the IC yet, probe all candidates on
     * every poll. The IC wakes on touch, so we'll catch it when
     * the user touches the screen.
     */
    if (!active_addr) {
        for (unsigned i = 0; i < NUM_CANDIDATES; i++) {
            uint8_t data[7];
            if (try_read(candidate_addrs[i], 0x00, data, 7)) {
                uint8_t fingers = data[2];
                if (fingers > 0) {
                    active_addr = candidate_addrs[i];
                    printf("[touch] discovered on touch at 0x%02X "
                           "reg[0..6]: %02X %02X %02X %02X %02X %02X %02X\n",
                           active_addr,
                           data[0], data[1], data[2], data[3],
                           data[4], data[5], data[6]);
                    configure_ic(active_addr);
                    break;
                }
            }
        }
        if (!active_addr) return;
    }

    uint8_t buf[7];
    if (!try_read(active_addr, 0x00, buf, 7))
        return;

    uint8_t gesture = buf[1];
    uint8_t fingers = buf[2];

    if (gesture != 0x00 || fingers > 0) {
        printf("[touch] gesture=0x%02X fingers=%u x=%u y=%u\n",
               gesture, fingers,
               ((uint16_t)(buf[3] & 0x0F) << 8) | buf[4],
               ((uint16_t)(buf[5] & 0x0F) << 8) | buf[6]);
    }

    /* Reconfigure if IC was sleeping and just woke (e.g. after sleep timeout) */
    if (fingers > 0 && !configured) {
        configure_ic(active_addr);
    }

    event->gesture = map_gesture(gesture);

    if (fingers > 0) {
        event->pressed = true;
        event->x = ((uint16_t)(buf[3] & 0x0F) << 8) | buf[4];
        event->y = ((uint16_t)(buf[5] & 0x0F) << 8) | buf[6];
    }
}
