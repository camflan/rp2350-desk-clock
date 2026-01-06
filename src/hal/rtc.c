// src/hal/rtc.c
#include "rtc.h"
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include <stdio.h>
#include <string.h>

// Current date and time
static rtc_datetime_t current_datetime = {
    .date = {2025, 1, 1, 3},  // 2025-01-01, Wednesday
    .time = {12, 0, 0}         // 12:00:00
};

// Repeating timer
static struct repeating_timer rtc_timer;

// Days in each month (non-leap year)
static const uint8_t days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// Day of week names
static const char* weekday_names[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday",
    "Thursday", "Friday", "Saturday"
};

// Check if year is leap year
static bool is_leap_year(uint16_t year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

// Get days in month for given year/month
static uint8_t get_days_in_month(uint16_t year, uint8_t month) {
    if (month == 2 && is_leap_year(year)) {
        return 29;
    }
    return days_in_month[month - 1];
}

// Timer callback - called every second
static bool rtc_tick_callback(struct repeating_timer *t) {
    // Increment seconds
    current_datetime.time.second++;

    if (current_datetime.time.second >= 60) {
        current_datetime.time.second = 0;
        current_datetime.time.minute++;

        if (current_datetime.time.minute >= 60) {
            current_datetime.time.minute = 0;
            current_datetime.time.hour++;

            if (current_datetime.time.hour >= 24) {
                current_datetime.time.hour = 0;
                current_datetime.date.day++;
                current_datetime.date.weekday = (current_datetime.date.weekday + 1) % 7;

                uint8_t max_days = get_days_in_month(current_datetime.date.year,
                                                      current_datetime.date.month);
                if (current_datetime.date.day > max_days) {
                    current_datetime.date.day = 1;
                    current_datetime.date.month++;

                    if (current_datetime.date.month > 12) {
                        current_datetime.date.month = 1;
                        current_datetime.date.year++;
                    }
                }
            }
        }
    }

    return true; // Continue repeating
}

void rtc_init(void) {
    printf("Initializing software RTC...\n");

    // Start a repeating timer that fires every 1000ms (1 second)
    add_repeating_timer_ms(1000, rtc_tick_callback, NULL, &rtc_timer);

    printf("RTC initialized: %04d-%02d-%02d %02d:%02d:%02d\n",
           current_datetime.date.year, current_datetime.date.month, current_datetime.date.day,
           current_datetime.time.hour, current_datetime.time.minute, current_datetime.time.second);
}

void rtc_set_time(const rtc_time_t *time) {
    if (time->hour < 24 && time->minute < 60 && time->second < 60) {
        current_datetime.time = *time;
    }
}

void rtc_set_date(const rtc_date_t *date) {
    if (date->year >= 2000 && date->month >= 1 && date->month <= 12 &&
        date->day >= 1 && date->weekday <= 6) {
        current_datetime.date = *date;
    }
}

void rtc_set_datetime(const rtc_datetime_t *datetime) {
    rtc_set_date(&datetime->date);
    rtc_set_time(&datetime->time);
}

void rtc_get_time(rtc_time_t *time) {
    *time = current_datetime.time;
}

void rtc_get_date(rtc_date_t *date) {
    *date = current_datetime.date;
}

void rtc_get_datetime(rtc_datetime_t *datetime) {
    *datetime = current_datetime;
}

void rtc_get_time_string(char *buffer, size_t size) {
    snprintf(buffer, size, "%02d:%02d:%02d",
             current_datetime.time.hour,
             current_datetime.time.minute,
             current_datetime.time.second);
}

void rtc_get_date_string(char *buffer, size_t size) {
    snprintf(buffer, size, "%04d-%02d-%02d",
             current_datetime.date.year,
             current_datetime.date.month,
             current_datetime.date.day);
}

const char* rtc_get_weekday_string(uint8_t weekday) {
    if (weekday > 6) return "Unknown";
    return weekday_names[weekday];
}
