#include <SDL2/SDL.h>
#include <stdlib.h>
#include <string.h>
#include "lvgl.h"
#include "display.h"
#include "app.h"
#include "navigation.h"

static const char *screenshot_path = NULL;
static const char *start_screen = NULL;
static int face_id = -1;

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--screenshot") == 0 && i + 1 < argc)
            screenshot_path = argv[++i];
        else if (strcmp(argv[i], "--screen") == 0 && i + 1 < argc)
            start_screen = argv[++i];
        else if (strcmp(argv[i], "--face") == 0 && i + 1 < argc)
            face_id = atoi(argv[++i]);
    }

    /* Software renderer needed for SDL_RenderReadPixels on macOS Metal */
    if (screenshot_path)
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");

    lv_init();

    lv_display_t *disp = lv_sdl_window_create(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_sdl_window_set_title(disp, "RP2350 Desk Clock Sim");

    lv_sdl_mouse_create();

    app_init();

    if (face_id >= 0) {
        #include "clock_analog.h"
        #include "settings.h"
        clock_analog_set_face_style((watch_face_id_t)face_id);
        /* Recreate the clock face with the new style */
        clock_analog_destroy();
        clock_analog_create();
        lv_screen_load(clock_analog_get_screen());
    }

    /* Navigate to requested screen */
    if (start_screen) {
        if (strcmp(start_screen, "settings") == 0)
            nav_show_settings();
        else if (strcmp(start_screen, "timeset") == 0)
            nav_show_time_set();
    }

    if (screenshot_path) {
        /* Render enough frames to finish animations */
        for (int i = 0; i < 200; i++) {
            lv_timer_handler();
            SDL_Delay(33);
        }

        /* Find the SDL window LVGL created */
        SDL_Window *win = NULL;
        SDL_Renderer *ren = NULL;
        for (uint32_t id = 1; id < 10; id++) {
            win = SDL_GetWindowFromID(id);
            if (win) {
                ren = SDL_GetRenderer(win);
                if (ren) break;
            }
        }
        if (ren) {
            SDL_RenderPresent(ren);
            int w, h;
            SDL_GetRendererOutputSize(ren, &w, &h);
            SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(
                0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);
            if (surf) {
                SDL_RenderReadPixels(ren, NULL, surf->format->format,
                                     surf->pixels, surf->pitch);
                SDL_SaveBMP(surf, screenshot_path);
                SDL_FreeSurface(surf);
                fprintf(stderr, "[screenshot] saved %dx%d to %s\n", w, h, screenshot_path);
            }
        } else {
            fprintf(stderr, "[screenshot] no renderer found\n");
        }
        return 0;
    }

    while (1) {
        uint32_t delay = lv_timer_handler();
        if (delay < 1) delay = 1;
        SDL_Delay(delay);
    }

    return 0;
}
