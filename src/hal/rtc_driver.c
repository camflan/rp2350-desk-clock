#include "rtc_driver.h"
#include "pico/stdlib.h"
#include <stdio.h>

static rtc_datetime_t base_dt;
static absolute_time_t base_ts;

static const char *weekday_names[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday",
    "Thursday", "Friday", "Saturday"
};

static const uint8_t days_in_month_table[] = {
    0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static bool is_leap_year(uint16_t y) {
    return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

static uint8_t days_in_month(uint16_t year, uint8_t month) {
    if (month == 2 && is_leap_year(year)) return 29;
    return days_in_month_table[month];
}

/* Tomohiko Sakamoto's day-of-week algorithm */
static uint8_t calc_dotw(uint16_t y, uint8_t m, uint8_t d) {
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (m < 3) y--;
    return (y + y/4 - y/100 + y/400 + t[m - 1] + d) % 7;
}

static void add_seconds(rtc_datetime_t *dt, uint64_t secs) {
    dt->second += secs % 60;
    secs /= 60;
    if (dt->second >= 60) { dt->second -= 60; secs++; }

    dt->minute += secs % 60;
    secs /= 60;
    if (dt->minute >= 60) { dt->minute -= 60; secs++; }

    dt->hour += secs % 24;
    secs /= 24;
    if (dt->hour >= 24) { dt->hour -= 24; secs++; }

    /* Remaining full days */
    while (secs > 0) {
        uint8_t dim = days_in_month(dt->year, dt->month);
        uint8_t remaining = dim - dt->day;
        if (secs <= remaining) {
            dt->day += secs;
            break;
        }
        secs -= (remaining + 1);
        dt->day = 1;
        dt->month++;
        if (dt->month > 12) {
            dt->month = 1;
            dt->year++;
        }
    }

    dt->dotw = calc_dotw(dt->year, dt->month, dt->day);
}

void rtc_app_init(void) {
    base_dt = (rtc_datetime_t){
        .year = 2026, .month = 3, .day = 18,
        .hour = 12, .minute = 0, .second = 0,
        .dotw = 3  // Wednesday
    };
    base_ts = get_absolute_time();
}

void rtc_app_get_datetime(rtc_datetime_t *dt) {
    int64_t elapsed_us = absolute_time_diff_us(base_ts, get_absolute_time());
    uint64_t elapsed_s = (uint64_t)(elapsed_us / 1000000);

    *dt = base_dt;
    if (elapsed_s > 0)
        add_seconds(dt, elapsed_s);
}

void rtc_app_set_datetime(const rtc_datetime_t *dt) {
    base_dt = *dt;
    base_ts = get_absolute_time();
}

void rtc_app_get_time_string(char *buf, size_t len) {
    rtc_datetime_t dt;
    rtc_app_get_datetime(&dt);
    snprintf(buf, len, "%02u:%02u:%02u", dt.hour, dt.minute, dt.second);
}

void rtc_app_get_date_string(char *buf, size_t len) {
    rtc_datetime_t dt;
    rtc_app_get_datetime(&dt);
    snprintf(buf, len, "%04u-%02u-%02u", dt.year, dt.month, dt.day);
}

const char *rtc_app_get_weekday_string(void) {
    rtc_datetime_t dt;
    rtc_app_get_datetime(&dt);
    return weekday_names[dt.dotw % 7];
}
