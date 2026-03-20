#ifndef APP_H
#define APP_H

/*
 * Shared application init — called by both hardware main() and
 * the desktop simulator after platform-specific setup is done.
 *
 * Expects: LVGL initialized, display created, indev registered.
 */
void app_init(void);

#endif
