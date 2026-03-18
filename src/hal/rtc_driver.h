#ifndef RTC_DRIVER_H
#define RTC_DRIVER_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint16_t year;
    uint8_t  month;   // 1-12
    uint8_t  day;     // 1-31
    uint8_t  hour;    // 0-23
    uint8_t  minute;  // 0-59
    uint8_t  second;  // 0-59
    uint8_t  dotw;    // 0=Sun, 1=Mon, …, 6=Sat
} rtc_datetime_t;

void             rtc_app_init(void);
void             rtc_app_get_datetime(rtc_datetime_t *dt);
void             rtc_app_set_datetime(const rtc_datetime_t *dt);
void             rtc_app_get_time_string(char *buf, size_t len);
void             rtc_app_get_date_string(char *buf, size_t len);
const char      *rtc_app_get_weekday_string(void);

#endif
