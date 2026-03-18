#ifndef SETTINGS_SCREEN_H
#define SETTINGS_SCREEN_H

#include "lvgl.h"

void       settings_screen_create(void);
void       settings_screen_destroy(void);
lv_obj_t  *settings_screen_get_screen(void);

/* Called when navigating back — caller provides the callback */
typedef void (*settings_back_cb_t)(void);
void settings_screen_set_back_cb(settings_back_cb_t cb);

#endif
