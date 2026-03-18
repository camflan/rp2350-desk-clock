#include <SDL2/SDL.h>
#include "lvgl.h"
#include "clock_analog.h"
#include "rtc_driver.h"
#include "display.h"

int main(void) {
    rtc_app_init();

    lv_init();

    lv_display_t *disp = lv_sdl_window_create(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_sdl_window_set_title(disp, "RP2350 Desk Clock Sim");

    clock_analog_create();

    while (1) {
        uint32_t delay = lv_timer_handler();
        if (delay < 1) delay = 1;
        SDL_Delay(delay);
    }

    return 0;
}
