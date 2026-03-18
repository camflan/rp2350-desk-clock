#ifndef NAVIGATION_H
#define NAVIGATION_H

#include "touch_driver.h"

/* Initialize navigation — call after all screens are created */
void nav_init(void);

/* Call from main loop with latest touch event */
void nav_handle_gesture(const touch_event_t *event);

/* Navigate to a specific screen */
void nav_show_clock(void);
void nav_show_settings(void);

#endif
