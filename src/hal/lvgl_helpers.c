// src/hal/lvgl_helpers.c
#include "lvgl_helpers.h"
#include "display_driver.h"
#include <stdio.h>

#define LVGL_BUFFER_SIZE (DISPLAY_WIDTH * 20)

static lv_color_t buf1[LVGL_BUFFER_SIZE];
static lv_color_t buf2[LVGL_BUFFER_SIZE];

void lvgl_init(void) {
    printf("Initializing LVGL...\n");

    lv_init();

    // Initialize display hardware
    display_init();

    // Create display driver
    lv_display_t *disp = lv_display_create(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_display_set_flush_cb(disp, display_flush_cb);
    lv_display_set_buffers(disp, buf1, buf2, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    printf("LVGL initialized\n");
}

void lvgl_task_handler(void) {
    lv_timer_handler();
}
