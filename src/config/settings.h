#ifndef SETTINGS_H
#define SETTINGS_H

#include "rtc_driver.h"
#include <stdbool.h>
#include <stdint.h>

#define SETTINGS_MAGIC 0xDE5CC10C

#define SETTINGS_DEFAULT_CLOCK_MODE 0   /* analog */
#define SETTINGS_DEFAULT_THEME_ID   0   /* dark */
#define SETTINGS_DEFAULT_BRIGHTNESS 80

typedef struct {
    uint32_t        magic;
    uint8_t         clock_mode;
    uint8_t         theme_id;
    uint8_t         brightness;
    uint8_t         _pad;
    rtc_datetime_t  last_datetime;
    uint32_t        crc32;
} settings_t;

void              settings_load(void);
void              settings_save(void);
const settings_t *settings_get(void);

void settings_set_clock_mode(uint8_t mode);
void settings_set_theme_id(uint8_t id);
void settings_set_brightness(uint8_t brightness);
void settings_set_last_datetime(const rtc_datetime_t *dt);

#endif
