#ifndef CLOCK_DIGITAL_H
#define CLOCK_DIGITAL_H

#include "lvgl.h"

void       clock_digital_create(void);
void       clock_digital_destroy(void);
void       clock_digital_update(void);
lv_obj_t  *clock_digital_get_screen(void);

#endif
