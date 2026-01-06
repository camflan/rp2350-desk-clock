// src/ui/analog_clock.c
#include "analog_clock.h"
#include "../hal/rtc.h"
#include <math.h>
#include <stdio.h>

#define SCREEN_DIAMETER 466
#define CLOCK_RADIUS 200
#define CLOCK_CENTER_X 233  // 466/2
#define CLOCK_CENTER_Y 233  // 466/2

#define HOUR_HAND_LENGTH 100
#define MINUTE_HAND_LENGTH 140
#define SECOND_HAND_LENGTH 160

#define HOUR_HAND_WIDTH 8
#define MINUTE_HAND_WIDTH 6
#define SECOND_HAND_WIDTH 5

// Clock UI elements
static lv_obj_t *clock_container = NULL;
static lv_obj_t *hour_hand = NULL;
static lv_obj_t *minute_hand = NULL;
static lv_obj_t *second_hand = NULL;
static lv_obj_t *center_dot = NULL;

float radians_to_degres(float rad) {
    return (rad / M_PI) * 180;
}

float degrees_to_radians(float degrees) {
    return degrees / (2 * M_PI);
}

float sixtieth_to_radians(float value) {
    return degrees_to_radians((value / 60) * 360);
}

float hour_to_radians(int hour) {
    return degrees_to_radians((hour / 12) * 360);
}

lv_obj_t* analog_clock_create(lv_obj_t *parent) {
    // Create container for the clock
    clock_container = lv_obj_create(parent);
    lv_obj_set_size(clock_container, SCREEN_DIAMETER, SCREEN_DIAMETER);
    lv_obj_center(clock_container);
    lv_obj_set_style_bg_color(clock_container, lv_color_black(), 0);
    lv_obj_set_style_border_width(clock_container, 0, 0);
    lv_obj_set_style_pad_all(clock_container, 0, 0);

    // Allow clock hands to render outside container bounds
    lv_obj_add_flag(clock_container, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

    // Draw hour markers
    static lv_style_t marker_style;
    lv_style_init(&marker_style);
    lv_style_set_line_color(&marker_style, lv_color_white());
    lv_style_set_line_width(&marker_style, 3);

    // Draw tick marks and numbers for all 12 hours
    for (int hour = 0; hour < 12; hour++) {
        float angle = (hour * 30 - 90) * M_PI / 180.0f; // Start from 12 o'clock (top)

        // Major hours: 12, 3, 6, 9 get numbers
        // if (hour == 0 || hour == 3 || hour == 6 || hour == 9) {
        if (false) {
            lv_obj_t *label = lv_label_create(clock_container);

            // Determine which number to show
            const char *number;
            if (hour == 0) number = "12";
            else if (hour == 3) number = "3";
            else if (hour == 6) number = "6";
            else number = "9";

            lv_label_set_text(label, number);
            lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(label, lv_color_white(), 0);

            // Position number slightly inside the clock radius
            int16_t label_radius = CLOCK_RADIUS - 25;
            int16_t x = CLOCK_CENTER_X + (int16_t)(label_radius * cosf(angle));
            int16_t y = CLOCK_CENTER_Y + (int16_t)(label_radius * sinf(angle));

            lv_obj_set_pos(label, x - 10, y - 10); // Center the label
        }
        // All other hours get tick marks
        else {
            lv_obj_t *tick = lv_line_create(clock_container);

            // Tick mark from edge inward
            int16_t outer_radius = CLOCK_RADIUS;
            int16_t inner_radius = CLOCK_RADIUS - 25;

            static lv_point_precise_t tick_points[12][2];
            tick_points[hour][0].x = CLOCK_CENTER_X + (int16_t)(inner_radius * cosf(angle));
            tick_points[hour][0].y = CLOCK_CENTER_Y + (int16_t)(inner_radius * sinf(angle));
            tick_points[hour][1].x = CLOCK_CENTER_X + (int16_t)(outer_radius * cosf(angle));
            tick_points[hour][1].y = CLOCK_CENTER_Y + (int16_t)(outer_radius * sinf(angle));

            lv_line_set_points(tick, tick_points[hour], 2);
            lv_obj_add_style(tick, &marker_style, 0);
        }
    }

    // Create clock hands
    // Hour hand
    static lv_style_t hour_hand_style;
    lv_style_init(&hour_hand_style);
    // lv_style_set_line_color(&hour_hand_style, lv_color_white());
    // lv_style_set_line_width(&hour_hand_style, HOUR_HAND_WIDTH);
    // lv_style_set_line_rounded(&hour_hand_style, true);
    //
    hour_hand = lv_line_create(clock_container);
    lv_obj_add_style(hour_hand, &hour_hand_style, 0);

    // // Minute hand
    static lv_style_t minute_hand_style;
    lv_style_init(&minute_hand_style);
    // lv_style_set_line_color(&minute_hand_style, lv_color_white());
    // lv_style_set_line_width(&minute_hand_style, MINUTE_HAND_WIDTH);
    // lv_style_set_line_rounded(&minute_hand_style, true);
    //
    minute_hand = lv_line_create(clock_container);
    lv_obj_add_style(minute_hand, &minute_hand_style, 0);

    // Second hand
    static lv_style_t second_hand_style;
    lv_style_init(&second_hand_style);
    lv_style_set_line_color(&second_hand_style, lv_color_make(255, 0, 0)); // Red
    lv_style_set_line_width(&second_hand_style, SECOND_HAND_WIDTH);
    lv_style_set_line_rounded(&second_hand_style, true);

    second_hand = lv_line_create(clock_container);
    lv_obj_center(second_hand);
    lv_obj_add_style(second_hand, &second_hand_style, 0);

    static lv_point_precise_t p[2] = {{10,100}, {20,200}};
    lv_line_set_points(second_hand, p, 2);


    // Create center dot
    center_dot = lv_obj_create(clock_container);
    lv_obj_set_size(center_dot, 12, 12);
    lv_obj_center(center_dot);
    lv_obj_set_style_bg_color(center_dot, lv_color_white(), 0);
    lv_obj_set_style_border_width(center_dot, 0, 0);
    lv_obj_set_style_radius(center_dot, LV_RADIUS_CIRCLE, 0);

    if(false) {
        // Debug: Draw 10x10 grid
        static lv_style_t grid_style;
        lv_style_init(&grid_style);
        lv_style_set_line_color(&grid_style, lv_color_make(255, 0, 0)); // Red
        lv_style_set_line_width(&grid_style, 1);

        // Draw 11 vertical lines
        for (int i = 0; i <= 10; i++) {
            lv_obj_t *line = lv_line_create(clock_container);
            int x = (SCREEN_DIAMETER * i) / 10;
            static lv_point_precise_t points_v[11][2];
            points_v[i][0].x = x;
            points_v[i][0].y = 0;
            points_v[i][1].x = x;
            points_v[i][1].y = SCREEN_DIAMETER;
            lv_line_set_points(line, points_v[i], 2);
            lv_obj_add_style(line, &grid_style, 0);
        }

        // Draw 11 horizontal lines
        for (int i = 0; i <= 10; i++) {
            lv_obj_t *line = lv_line_create(clock_container);
            int y = (SCREEN_DIAMETER * i) / 10;
            static lv_point_precise_t points_h[11][2];
            points_h[i][0].x = 0;
            points_h[i][0].y = y;
            points_h[i][1].x = SCREEN_DIAMETER;
            points_h[i][1].y = y;
            lv_line_set_points(line, points_h[i], 2);
            lv_obj_add_style(line, &grid_style, 0);
        }
    }

    // Initial update
    analog_clock_update(clock_container);

    return clock_container;
}

void analog_clock_update(lv_obj_t *clock_obj) {
    if (!clock_obj) {
        printf("No clock…");
        return;}
    if (!hour_hand) {
        printf("No hour hand…");
        return;}
    if (!minute_hand) {
        printf("No minute hand…");
        return;}
    if (!second_hand) {
        printf("No second hand…");
        return;}

    // Get current time
    rtc_time_t time;
    rtc_get_time(&time);

    printf("\nUpdating time…\n");

    // Calculate hand angles (in radians)
    // Hour hand: 30 degrees per hour, plus offset for minutes
    // float hour_angle = ((time.hour % 12) * 30.0f + time.minute * 0.5f - 90.0f) * M_PI / 180.0f;
    float hour_angle = hour_to_radians(time.hour % 12);
    // Minute hand: 6 degrees per minute
    float minute_angle = sixtieth_to_radians(time.minute);
    // Second hand: 6 degrees per second
    float second_angle = sixtieth_to_radians(time.second);

    printf("Hour angle: %f\n", radians_to_degres(hour_angle));
    printf("Minute angle: %f\n", radians_to_degres(minute_angle));
    printf("Second (%f) angle: %f\n\n", time.second, radians_to_degres(second_angle));

    // Update hour hand
    // Points are relative to the line object, starting from (0,0)
    static lv_point_precise_t hour_points[2];
    // Update hour hand
    hour_points[0].x = 0;
    hour_points[0].y = 0;
    hour_points[1].x = (int16_t)(HOUR_HAND_LENGTH * cosf(hour_angle));
    hour_points[1].y = (int16_t)(HOUR_HAND_LENGTH * sinf(hour_angle));

    lv_line_set_points(hour_hand, hour_points, 2);
    lv_obj_center(hour_hand);


    // Update minute hand
    static lv_point_precise_t minute_points[2];
    minute_points[0].x = 0;
    minute_points[0].y = 0;
    minute_points[1].x = (int16_t)(MINUTE_HAND_LENGTH * cosf(minute_angle));
    minute_points[1].y = (int16_t)(MINUTE_HAND_LENGTH * sinf(minute_angle));
    lv_line_set_points(minute_hand, minute_points, 2);
    lv_obj_center(minute_hand);

    // Update second hand
    static lv_point_precise_t second_points[2];
    second_points[0].x = 0;
    second_points[0].y = 0;
    second_points[1].x =(int16_t)(SECOND_HAND_LENGTH * cosf(second_angle));
    second_points[1].y =(int16_t)(SECOND_HAND_LENGTH * sinf(second_angle));
    lv_line_set_points(second_hand, second_points, 2);
    lv_obj_center(second_hand);
}
