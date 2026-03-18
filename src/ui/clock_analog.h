#ifndef CLOCK_ANALOG_H
#define CLOCK_ANALOG_H

#include "lvgl.h"
#include "sweep.h"

void       clock_analog_create(void);
void       clock_analog_update(void);
lv_obj_t  *clock_analog_get_screen(void);
void       clock_analog_set_sweep_mode(sweep_mode_t mode);

#endif
