// src/hal/rtc.h
#ifndef RTC_H
#define RTC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Time structure
typedef struct {
    uint8_t hour;    // 0-23
    uint8_t minute;  // 0-59
    uint8_t second;  // 0-59
} rtc_time_t;

// Date structure
typedef struct {
    uint16_t year;   // e.g., 2025
    uint8_t month;   // 1-12
    uint8_t day;     // 1-31
    uint8_t weekday; // 0=Sunday, 1=Monday, ..., 6=Saturday
} rtc_date_t;

// Combined date/time structure
typedef struct {
    rtc_date_t date;
    rtc_time_t time;
} rtc_datetime_t;

// Initialize the software RTC
void rtc_init(void);

// Set current time
void rtc_set_time(const rtc_time_t *time);

// Set current date
void rtc_set_date(const rtc_date_t *date);

// Set both date and time
void rtc_set_datetime(const rtc_datetime_t *datetime);

// Get current time
void rtc_get_time(rtc_time_t *time);

// Get current date
void rtc_get_date(rtc_date_t *date);

// Get both date and time
void rtc_get_datetime(rtc_datetime_t *datetime);

// Get time as formatted string (HH:MM:SS)
void rtc_get_time_string(char *buffer, size_t size);

// Get date as formatted string (YYYY-MM-DD)
void rtc_get_date_string(char *buffer, size_t size);

// Get day of week as string
const char* rtc_get_weekday_string(uint8_t weekday);

#endif
