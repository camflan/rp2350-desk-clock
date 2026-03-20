#include "pico/stdlib.h"
#include "pico/time.h"
#include <stdio.h>
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

/*
 * CO5300 requires 2-byte aligned column/row addresses.
 * Round invalidated areas to even boundaries before LVGL renders.
 */
static void rounder_cb(lv_event_t *e) {
    lv_area_t *area = lv_event_get_param(e);
    area->x1 = (area->x1 >> 1) << 1;
    area->y1 = (area->y1 >> 1) << 1;
    area->x2 = ((area->x2 >> 1) << 1) + 1;
    area->y2 = ((area->y2 >> 1) << 1) + 1;
}

static void touch_indev_read_cb(lv_indev_t *indev, lv_indev_data_t *data) {
    (void)indev;
    static int32_t last_x, last_y;

    touch_event_t evt;
    touch_read(&evt);

    if (evt.pressed) {
        last_x = evt.x;
        last_y = evt.y;
    }

    data->point.x = last_x;
    data->point.y = last_y;
    data->state = evt.pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

int main(void) {
    stdio_init_all();
    sleep_ms(1000);
    printf("[main] desk_clock boot\n");

    settings_load();
    const settings_t *s = settings_get();

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
    lv_display_add_event_cb(disp, rounder_cb, LV_EVENT_INVALIDATE_AREA, NULL);

    static uint8_t buf1[DISPLAY_BUF_PIXELS * 2];
    static uint8_t buf2[DISPLAY_BUF_PIXELS * 2];
    lv_display_set_buffers(disp, buf1, buf2, sizeof(buf1),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_indev_t *touch_indev = lv_indev_create();
    lv_indev_set_type(touch_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touch_indev, touch_indev_read_cb);

    theme_init();
    theme_apply(s->theme_id);

    nav_init();

    while (1) {
        lv_timer_handler();
        sleep_ms(5);
    }

    return 0;
}
