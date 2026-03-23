/**
 * Stubs for hardware-dependent modules in the desktop simulator.
 * Provides no-op implementations so UI code links without real HW.
 */
#include "settings.h"
#include "display_driver.h"
#include "touch_driver.h"
#include <string.h>

/* ── Settings (in-memory only, no flash) ──────────────────── */

static settings_t sim_settings;
static bool sim_settings_loaded = false;

static void sim_settings_defaults(void) {
    memset(&sim_settings, 0, sizeof(sim_settings));
    sim_settings.magic      = SETTINGS_MAGIC;
    sim_settings.clock_mode = SETTINGS_DEFAULT_CLOCK_MODE;
    sim_settings.theme_id   = SETTINGS_DEFAULT_THEME_ID;
    sim_settings.brightness = SETTINGS_DEFAULT_BRIGHTNESS;
    sim_settings.sweep_mode    = SETTINGS_DEFAULT_SWEEP_MODE;
    sim_settings.watch_face_id = SETTINGS_DEFAULT_FACE_ID;
}

void settings_load(void) {
    if (!sim_settings_loaded) {
        sim_settings_defaults();
        sim_settings_loaded = true;
    }
}

void settings_save(void) {
    /* no-op in sim */
}

const settings_t *settings_get(void) {
    return &sim_settings;
}

void settings_set_clock_mode(uint8_t mode)  { sim_settings.clock_mode = mode; }
void settings_set_theme_id(uint8_t id)      { sim_settings.theme_id = id; }
void settings_set_brightness(uint8_t b)     { sim_settings.brightness = b; }
void settings_set_sweep_mode(uint8_t mode)  { sim_settings.sweep_mode = mode; }
void settings_set_watch_face_id(uint8_t id) { sim_settings.watch_face_id = id; }
void settings_set_last_datetime(const rtc_datetime_t *dt) {
    sim_settings.last_datetime = *dt;
}

/* ── Display driver (brightness only) ─────────────────────── */

void display_init(void) {}
void display_set_brightness(uint8_t brightness) { (void)brightness; }
void display_test_fill(uint16_t color) { (void)color; }
void display_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px) {
    (void)disp; (void)area; (void)px;
}

/* ── Touch driver (no touch in sim — SDL handles input) ───── */

void touch_init(void) {}
void touch_read(touch_event_t *event) {
    memset(event, 0, sizeof(*event));
}
