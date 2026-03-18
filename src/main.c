#include "pico/stdlib.h"
#include "pico/time.h"
#include "lvgl.h"
#include "display_driver.h"
#include "display.h"
#include "rtc_driver.h"
#include "clock_analog.h"

static uint32_t tick_cb(void) {
    return (uint32_t)to_ms_since_boot(get_absolute_time());
}

int main(void) {
    stdio_init_all();
    display_init();
    rtc_app_init();

    lv_init();
    lv_tick_set_cb(tick_cb);

    lv_display_t *disp = lv_display_create(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_display_set_flush_cb(disp, display_flush_cb);

    static uint8_t buf1[DISPLAY_BUF_PIXELS * 2];
    static uint8_t buf2[DISPLAY_BUF_PIXELS * 2];
    lv_display_set_buffers(disp, buf1, buf2, sizeof(buf1),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    clock_analog_create();

    while (1) {
        lv_timer_handler();
        sleep_ms(5);
    }

    return 0;
}
