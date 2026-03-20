#include <SDL2/SDL.h>
#include "lvgl.h"
#include "display.h"
#include "app.h"

static const char *screenshot_path = NULL;

int main(int argc, char *argv[]) {
    /* --screenshot <path> : render a few frames then save a BMP and exit */
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "--screenshot") == 0)
            screenshot_path = argv[i + 1];
    }

    lv_init();

    lv_display_t *disp = lv_sdl_window_create(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_sdl_window_set_title(disp, "RP2350 Desk Clock Sim");

    lv_sdl_mouse_create();

    app_init();

    if (screenshot_path) {
        /* Render several frames to let LVGL fully draw */
        for (int i = 0; i < 10; i++)
            lv_timer_handler();

        SDL_Window *win = SDL_GetWindowFromID(1);
        if (win) {
            SDL_Renderer *ren = SDL_GetRenderer(win);
            if (ren) {
                int w, h;
                SDL_GetRendererOutputSize(ren, &w, &h);
                SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(
                    0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);
                if (surf) {
                    SDL_RenderReadPixels(ren, NULL, surf->format->format,
                                         surf->pixels, surf->pitch);
                    SDL_SaveBMP(surf, screenshot_path);
                    SDL_FreeSurface(surf);
                }
            }
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
