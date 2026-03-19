#include "pico/stdlib.h"
#include "pico/time.h"
#include "lvgl.h"
#include "display_driver.h"
#include "display.h"
#include "rtc_driver.h"
#include "touch_driver.h"
#include "settings.h"
#include "theme.h"
#include "navigation.h"

static uint32_t tick_cb(void) {
    return (uint32_t)to_ms_since_boot(get_absolute_time());
}

int main(void) {
    stdio_init_all();

    /* Load persisted settings (or defaults if flash is blank) */
    settings_load();
    const settings_t *s = settings_get();

    /* Restore saved time if valid (magic was validated by settings_load) */
    rtc_app_init();
    if (s->last_datetime.year >= 2024)
        rtc_app_set_datetime(&s->last_datetime);

    display_init();
    display_set_brightness(s->brightness);
    touch_init();

    lv_init();
    lv_tick_set_cb(tick_cb);

    lv_display_t *disp = lv_display_create(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_display_set_flush_cb(disp, display_flush_cb);

    static uint8_t buf1[DISPLAY_BUF_PIXELS * 2];
    static uint8_t buf2[DISPLAY_BUF_PIXELS * 2];
    lv_display_set_buffers(disp, buf1, buf2, sizeof(buf1),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    /* Initialize theme system and apply saved theme */
    theme_init();
    theme_apply(s->theme_id);

    /* Navigation creates and loads the appropriate clock face */
    nav_init();

    while (1) {
        touch_event_t evt;
        touch_read(&evt);
        nav_handle_gesture(&evt);

        lv_timer_handler();
        sleep_ms(5);
    }

    return 0;
}
