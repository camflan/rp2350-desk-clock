#include "clock_analog.h"
#include "display.h"
#include "rtc_driver.h"
#include <math.h>

#define CENTER_X (DISPLAY_WIDTH / 2)
#define CENTER_Y (DISPLAY_HEIGHT / 2)

#define HOUR_HAND_LEN   100
#define MINUTE_HAND_LEN 150
#define SECOND_HAND_LEN 170
#define MARKER_OUTER    220
#define MARKER_INNER    200

#define HOUR_HAND_WIDTH   6
#define MINUTE_HAND_WIDTH 4
#define SECOND_HAND_WIDTH 2
#define MARKER_WIDTH      2

#define NUM_MARKERS 12
#define DEG_TO_RAD  (M_PI / 180.0)

/* 12 o'clock is up (-90°) */
#define ANGLE_OFFSET (-90.0)

static lv_obj_t *screen;
static lv_obj_t *hour_hand;
static lv_obj_t *minute_hand;
static lv_obj_t *second_hand;
static lv_obj_t *date_label;

static lv_point_precise_t hour_pts[2];
static lv_point_precise_t minute_pts[2];
static lv_point_precise_t second_pts[2];
static lv_point_precise_t marker_pts[NUM_MARKERS][2];

static int8_t last_second = -1;

static void hand_endpoint(double angle_deg, int length,
                          lv_coord_t *x, lv_coord_t *y) {
    double rad = (angle_deg + ANGLE_OFFSET) * DEG_TO_RAD;
    *x = (lv_coord_t)(CENTER_X + length * cos(rad));
    *y = (lv_coord_t)(CENTER_Y + length * sin(rad));
}

static lv_obj_t *create_line(lv_obj_t *parent, lv_point_precise_t *pts,
                             lv_color_t color, int width) {
    lv_obj_t *line = lv_line_create(parent);
    lv_line_set_points(line, pts, 2);
    lv_obj_set_style_line_color(line, color, 0);
    lv_obj_set_style_line_width(line, width, 0);
    lv_obj_set_style_line_rounded(line, true, 0);
    return line;
}

static void create_markers(lv_obj_t *parent) {
    for (int i = 0; i < NUM_MARKERS; i++) {
        double angle = i * 30.0;
        lv_coord_t x1, y1, x2, y2;
        hand_endpoint(angle, MARKER_INNER, &x1, &y1);
        hand_endpoint(angle, MARKER_OUTER, &x2, &y2);

        marker_pts[i][0] = (lv_point_precise_t){x1, y1};
        marker_pts[i][1] = (lv_point_precise_t){x2, y2};

        int w = (i % 3 == 0) ? MARKER_WIDTH + 2 : MARKER_WIDTH;
        create_line(parent, marker_pts[i], lv_color_white(), w);
    }
}

static void update_hand(lv_obj_t *hand, lv_point_precise_t *pts,
                        double angle_deg, int length) {
    lv_coord_t ex, ey;
    hand_endpoint(angle_deg, length, &ex, &ey);
    pts[0] = (lv_point_precise_t){CENTER_X, CENTER_Y};
    pts[1] = (lv_point_precise_t){ex, ey};
    lv_line_set_points(hand, pts, 2);
}

static void timer_cb(lv_timer_t *timer) {
    (void)timer;
    clock_analog_update();
}

void clock_analog_create(void) {
    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    create_markers(screen);

    hour_hand = create_line(screen, hour_pts,
                            lv_color_white(), HOUR_HAND_WIDTH);
    minute_hand = create_line(screen, minute_pts,
                              lv_color_white(), MINUTE_HAND_WIDTH);
    second_hand = create_line(screen, second_pts,
                              lv_color_make(0xFF, 0x40, 0x40), SECOND_HAND_WIDTH);

    date_label = lv_label_create(screen);
    lv_obj_set_style_text_color(date_label, lv_color_white(), 0);
    lv_obj_align(date_label, LV_ALIGN_CENTER, 0, 60);

    clock_analog_update();
    lv_screen_load(screen);

    lv_timer_create(timer_cb, 1000, NULL);
}

void clock_analog_update(void) {
    rtc_datetime_t dt;
    rtc_app_get_datetime(&dt);

    if (dt.second == last_second)
        return;
    last_second = dt.second;

    double hour_angle   = (dt.hour % 12) * 30.0 + dt.minute * 0.5;
    double minute_angle = dt.minute * 6.0 + dt.second * 0.1;
    double second_angle = dt.second * 6.0;

    update_hand(hour_hand,   hour_pts,   hour_angle,   HOUR_HAND_LEN);
    update_hand(minute_hand, minute_pts, minute_angle, MINUTE_HAND_LEN);
    update_hand(second_hand, second_pts, second_angle, SECOND_HAND_LEN);

    char buf[32];
    rtc_app_get_date_string(buf, sizeof(buf));
    lv_label_set_text(date_label, buf);

    /* Force full screen redraw to avoid partial invalidation artifacts */
    lv_obj_invalidate(screen);
}

lv_obj_t *clock_analog_get_screen(void) {
    return screen;
}
