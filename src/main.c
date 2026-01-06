// src/main.c
#include <stdio.h>
#include "pico/stdlib.h"
#include "lvgl.h"
#include "hal/lvgl_helpers.h"
#include "hal/display_driver.h"
#include "hal/rtc.h"
#include "ui/analog_clock.h"

int main() {
    stdio_init_all();

    // Give USB time to enumerate
    sleep_ms(2000);

    printf("RP2350 AMOLED Clock starting…\n");
    printf("USB serial initialized\n");

    // Initialize RTC
    rtc_init();

    // Set initial time (you can change this to current time)
    rtc_datetime_t initial_time = {
        .date = {2025, 12, 19, 5},  // 2025-12-19, Friday
        .time = {14, 30, 0}          // 14:30:00 (2:30 PM)
    };
    rtc_set_datetime(&initial_time);

    lvgl_init();

    printf("Creating analog clock UI…\n");

    // Set black background
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), 0);

    // Create analog clock
    lv_obj_t *clock = analog_clock_create(lv_screen_active());
    printf("Analog clock created!\n");

    printf("Entering main loop…\n");

    uint32_t last_update = 0;
    while (1) {
        // Update clock every 100ms
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - last_update >= 100) {
            analog_clock_update(clock);
            last_update = now;
        }

        lvgl_task_handler();
        sleep_ms(10);
    }

    return 0;
}
