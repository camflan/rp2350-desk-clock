#include "app.h"
#include "settings.h"
#include "rtc_driver.h"
#include "display_driver.h"
#include "clock_analog.h"
#include "theme.h"
#include "navigation.h"
#include "lvgl.h"

#define TIME_SAVE_INTERVAL_MS (30 * 60 * 1000)  /* 30 minutes */

static void time_save_cb(lv_timer_t *timer) {
    (void)timer;
    rtc_datetime_t now;
    rtc_app_get_datetime(&now);
    settings_set_last_datetime(&now);
    settings_save();
}

void app_init(void) {
    settings_load();
    const settings_t *s = settings_get();

    rtc_app_init();
    if (s->last_datetime.year >= 2024)
        rtc_app_set_datetime(&s->last_datetime);

    display_set_brightness(s->brightness);

    theme_init();
    theme_apply(s->theme_id);

    nav_init();

    clock_analog_set_sweep_mode((sweep_mode_t)s->sweep_mode);

    lv_timer_create(time_save_cb, TIME_SAVE_INTERVAL_MS, NULL);
}
