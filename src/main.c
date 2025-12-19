// src/main.c
#include <stdio.h>
#include "pico/stdlib.h"
#include "lvgl.h"
#include "hal/lvgl_helpers.h"

int main() {
    stdio_init_all();

    printf("RP2350 AMOLED Clock starting...\n");

    lvgl_init();

    // Create a simple test screen
    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hello LVGL!");
    lv_obj_center(label);

    printf("Entering main loop...\n");

    while (1) {
        lvgl_task_handler();
        sleep_ms(5);
    }

    return 0;
}
