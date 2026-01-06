// src/ui/analog_clock.h
#ifndef ANALOG_CLOCK_H
#define ANALOG_CLOCK_H

#include "lvgl.h"

// Create and initialize the analog clock screen
lv_obj_t* analog_clock_create(lv_obj_t *parent);

// Update the clock hands based on current time
void analog_clock_update(lv_obj_t *clock_obj);

#endif
