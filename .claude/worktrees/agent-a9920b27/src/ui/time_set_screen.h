#ifndef TIME_SET_SCREEN_H
#define TIME_SET_SCREEN_H

#include "lvgl.h"

void       time_set_screen_create(void);
void       time_set_screen_destroy(void);
lv_obj_t  *time_set_screen_get_screen(void);

typedef void (*time_set_done_cb_t)(void);
void time_set_screen_set_done_cb(time_set_done_cb_t cb);

#endif
