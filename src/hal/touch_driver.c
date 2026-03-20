/*
 * Touch driver for Waveshare RP2350-Touch-AMOLED-1.43
 *
 * Pin assignments from board schematic V2.0:
 *   SDA=GPIO6  SCL=GPIO7  INT=GPIO21  RST=GPIO22
 *   Bus: i2c1 @ 400kHz
 *
 * The touch IC (FT6236 family) enters deep sleep quickly after
 * reset. It only wakes on touch events. We must try multiple
 * candidate addresses and configure it in the brief post-reset
 * window or on first touch.
 *
 * Gestures are NOT read from the IC — FT6236 hardware gesture
 * register is unreliable. LVGL computes gestures from raw
 * coordinates via lv_indev.
 *
 *  Register layout (read from 0x00):
 *    [0] mode  [1] gesture  [2] fingers
 *    [3] x_hi  [4] x_lo     [5] y_hi    [6] y_lo
 */

#include "touch_driver.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include <stdio.h>

#ifdef NDEBUG
#define TOUCH_LOG(...)
#else
#define TOUCH_LOG(...) printf(__VA_ARGS__)
#endif

#define REG_FINGERS     0x02
#define REG_X_HI        0x03
#define REG_X_LO        0x04
#define REG_Y_HI        0x05
#define REG_Y_LO        0x06
#define REG_CHIP_ID     0xA7
#define REG_AUTO_SLEEP  0xFE
#define REG_IRQ_CTL     0xFA

/* Candidate I2C addresses for FT6236 / CST816S family */
static const uint8_t candidate_addrs[] = {0x38, 0x15, 0x5A, 0x51};
#define NUM_CANDIDATES (sizeof(candidate_addrs) / sizeof(candidate_addrs[0]))

static uint8_t active_addr = 0;  /* 0 = not yet discovered */
static bool configured = false;

#define I2C_TIMEOUT_US 10000

static bool try_read(uint8_t addr, uint8_t reg, uint8_t *buf, size_t len) {
    absolute_time_t deadline = make_timeout_time_us(I2C_TIMEOUT_US);
    int ret = i2c_write_blocking_until(TOUCH_I2C_INST, addr, &reg, 1, true, deadline);
    if (ret < 0) return false;
    deadline = make_timeout_time_us(I2C_TIMEOUT_US);
    ret = i2c_read_blocking_until(TOUCH_I2C_INST, addr, buf, len, false, deadline);
    return ret >= 0;
}

static void try_write(uint8_t addr, uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    absolute_time_t deadline = make_timeout_time_us(I2C_TIMEOUT_US);
    i2c_write_blocking_until(TOUCH_I2C_INST, addr, buf, 2, false, deadline);
}

static void configure_ic(uint8_t addr) {
    try_write(addr, REG_AUTO_SLEEP, 0x01);  /* disable auto-sleep */
    try_write(addr, REG_IRQ_CTL, 0x71);     /* enable touch IRQ */
    configured = true;
    TOUCH_LOG("[touch] configured addr=0x%02X\n", addr);
}

/*
 * Reset the touch IC and immediately try to find it before it
 * enters deep sleep (~5-10ms window after reset).
 */
static void reset_and_probe(void) {
    gpio_put(TOUCH_PIN_RST, 0);
    sleep_ms(5);
    gpio_put(TOUCH_PIN_RST, 1);
    sleep_ms(5);

    for (unsigned i = 0; i < NUM_CANDIDATES; i++) {
        uint8_t chip_id = 0;
        if (try_read(candidate_addrs[i], REG_CHIP_ID, &chip_id, 1)) {
            TOUCH_LOG("[touch] found IC at 0x%02X, chip_id=0x%02X\n",
                   candidate_addrs[i], chip_id);
            active_addr = candidate_addrs[i];
            configure_ic(active_addr);
            return;
        }
    }
    TOUCH_LOG("[touch] no IC found after reset (will retry on touch)\n");
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

    /* Late discovery: probe candidates if IC wasn't found at boot */
    if (!active_addr) {
        for (unsigned i = 0; i < NUM_CANDIDATES; i++) {
            uint8_t fingers = 0;
            if (try_read(candidate_addrs[i], REG_FINGERS, &fingers, 1)
                && fingers > 0) {
                active_addr = candidate_addrs[i];
                TOUCH_LOG("[touch] discovered at 0x%02X\n", active_addr);
                configure_ic(active_addr);
                break;
            }
        }
        if (!active_addr) return;
    }

    uint8_t buf[5];
    if (!try_read(active_addr, REG_FINGERS, buf, 5))
        return;

    uint8_t fingers = buf[0];

    if (fingers > 0 && !configured)
        configure_ic(active_addr);

    if (fingers > 0) {
        event->pressed = true;
        event->x = ((uint16_t)(buf[1] & 0x0F) << 8) | buf[2];
        event->y = ((uint16_t)(buf[3] & 0x0F) << 8) | buf[4];
    }
}
