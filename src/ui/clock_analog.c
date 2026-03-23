#include "clock_analog.h"
#include "display.h"
#include "rtc_driver.h"
#include "sweep.h"
#include "watch_face_style.h"
#include <math.h>

#define CENTER_X (DISPLAY_WIDTH / 2)
#define CENTER_Y (DISPLAY_HEIGHT / 2)

#define NUM_MARKERS    12
#define NUM_BEZEL_TICKS 60
#define DEG_TO_RAD     (M_PI / 180.0)
#define ANGLE_OFFSET   (-90.0)
#define AA_PAD         2

/* Bezel geometry (FXD) */
#define BEZEL_OUTER    230
#define BEZEL_INNER    222
#define BEZEL_5MIN     218
#define BEZEL_12       214

static const watch_face_style_t *active_style;
static sweep_mode_t current_sweep = SWEEP_1HZ;
static lv_timer_t  *update_timer;

static lv_obj_t *screen;
static lv_obj_t *hour_hand;
static lv_obj_t *minute_hand;
static lv_obj_t *second_hand;
static lv_obj_t *date_label;

static lv_point_precise_t hour_pts[2];
static lv_point_precise_t minute_pts[2];
static lv_point_precise_t second_pts[2];
static lv_point_precise_t marker_pts[NUM_MARKERS][2];
static lv_point_precise_t bezel_pts[NUM_BEZEL_TICKS][2];

static int8_t   last_second = -1;
static int8_t   last_minute = -1;
static int8_t   last_day    = -1;
static double   last_second_angle = -1.0;
static uint32_t second_start_tick;

static inline int32_t min32(int32_t a, int32_t b) { return a < b ? a : b; }
static inline int32_t max32(int32_t a, int32_t b) { return a > b ? a : b; }

static void invalidate_hand_area(lv_obj_t *parent,
                                 lv_point_precise_t *pts,
                                 int line_width) {
    int32_t pad = line_width / 2 + AA_PAD;
    lv_area_t area;
    area.x1 = min32((int32_t)pts[0].x, (int32_t)pts[1].x) - pad;
    area.y1 = min32((int32_t)pts[0].y, (int32_t)pts[1].y) - pad;
    area.x2 = max32((int32_t)pts[0].x, (int32_t)pts[1].x) + pad;
    area.y2 = max32((int32_t)pts[0].y, (int32_t)pts[1].y) + pad;
    lv_obj_invalidate_area(parent, &area);
}

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
    const watch_face_style_t *s = active_style;
    for (int i = 0; i < NUM_MARKERS; i++) {
        double angle = i * 30.0;
        lv_coord_t x1, y1, x2, y2;
        hand_endpoint(angle, s->marker_inner, &x1, &y1);
        hand_endpoint(angle, s->marker_outer, &x2, &y2);

        marker_pts[i][0] = (lv_point_precise_t){x1, y1};
        marker_pts[i][1] = (lv_point_precise_t){x2, y2};

        bool major = (i % 3 == 0);
        int w = major ? s->marker_major_width : s->marker_width;
        lv_color_t color = major ? s->marker_primary : s->marker_secondary;
        create_line(parent, marker_pts[i], color, w);
    }
}

/*
 * FXD-style dive bezel: 60 minute ticks around the outer edge.
 *
 *   Regular tick:    BEZEL_INNER → BEZEL_OUTER  (1px)
 *   Every 5 min:     BEZEL_5MIN → BEZEL_OUTER  (2px)
 *   12 o'clock:      BEZEL_12   → BEZEL_OUTER  (3px, triangle-ish)
 */
static void create_bezel(lv_obj_t *parent) {
    const watch_face_style_t *s = active_style;
    int offset = s->bezel_offset_deg;

    for (int i = 0; i < NUM_BEZEL_TICKS; i++) {
        double angle = i * 6.0 + offset;
        int inner, w;
        lv_color_t color;

        if (i == 0) {
            inner = BEZEL_12;
            w = 3;
            color = s->marker_primary;
        } else if (i % 5 == 0) {
            inner = BEZEL_5MIN;
            w = 2;
            color = s->marker_primary;
        } else {
            inner = BEZEL_INNER;
            w = 1;
            color = s->marker_secondary;
        }

        lv_coord_t x1, y1, x2, y2;
        hand_endpoint(angle, inner, &x1, &y1);
        hand_endpoint(angle, BEZEL_OUTER, &x2, &y2);

        bezel_pts[i][0] = (lv_point_precise_t){x1, y1};
        bezel_pts[i][1] = (lv_point_precise_t){x2, y2};
        create_line(parent, bezel_pts[i], color, w);
    }
}

static void update_hand(lv_obj_t *hand, lv_point_precise_t *pts,
                        double angle_deg, int length, int line_width) {
    invalidate_hand_area(screen, pts, line_width);

    lv_coord_t ex, ey;
    hand_endpoint(angle_deg, length, &ex, &ey);
    pts[0] = (lv_point_precise_t){CENTER_X, CENTER_Y};
    pts[1] = (lv_point_precise_t){ex, ey};
    lv_line_set_points(hand, pts, 2);

    invalidate_hand_area(screen, pts, line_width);
}

static void timer_cb(lv_timer_t *timer) {
    (void)timer;
    clock_analog_update();
}

void clock_analog_create(void) {
    if (update_timer) {
        lv_timer_delete(update_timer);
        update_timer = NULL;
    }

    last_second = -1;
    last_minute = -1;
    last_day    = -1;
    last_second_angle = -1.0;

    if (!active_style)
        active_style = watch_face_get_style(FACE_CLASSIC);

    const watch_face_style_t *s = active_style;

    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, s->bg, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    if (s->show_bezel)
        create_bezel(screen);

    create_markers(screen);

    hour_hand   = create_line(screen, hour_pts,   s->hand_color,   s->hour_hand_width);
    minute_hand = create_line(screen, minute_pts,  s->hand_color,   s->minute_hand_width);
    second_hand = create_line(screen, second_pts,  s->second_color, s->second_hand_width);

    if (s->show_date) {
        date_label = lv_label_create(screen);
        lv_obj_set_style_text_color(date_label, s->date_color, 0);
        lv_obj_align(date_label, LV_ALIGN_CENTER, 0, 60);
    } else {
        date_label = NULL;
    }

    lv_obj_invalidate(screen);
    clock_analog_update();

    update_timer = lv_timer_create(timer_cb,
                                   sweep_interval_ms(current_sweep), NULL);
}

void clock_analog_update(void) {
    const watch_face_style_t *s = active_style;
    if (!s) return;

    rtc_datetime_t dt;
    rtc_app_get_datetime(&dt);

    if (dt.second != last_second) {
        last_second = dt.second;
        second_start_tick = lv_tick_get();
    }

    uint32_t elapsed_ms = lv_tick_get() - second_start_tick;
    if (elapsed_ms > 999) elapsed_ms = 999;

    double second_angle = sweep_second_angle(current_sweep, dt.second,
                                             elapsed_ms);

    if (second_angle != last_second_angle) {
        last_second_angle = second_angle;
        update_hand(second_hand, second_pts, second_angle,
                    s->second_hand_len, s->second_hand_width);
    }

    if (dt.minute != last_minute) {
        last_minute = dt.minute;

        double hour_angle   = (dt.hour % 12) * 30.0 + dt.minute * 0.5;
        double minute_angle = dt.minute * 6.0 + dt.second * 0.1;

        update_hand(hour_hand,   hour_pts,   hour_angle,
                    s->hour_hand_len,   s->hour_hand_width);
        update_hand(minute_hand, minute_pts, minute_angle,
                    s->minute_hand_len, s->minute_hand_width);
    }

    if (date_label && dt.day != last_day) {
        last_day = dt.day;
        lv_obj_invalidate(date_label);
        char buf[32];
        rtc_app_get_date_string(buf, sizeof(buf));
        lv_label_set_text(date_label, buf);
    }
}

void clock_analog_destroy(void) {
    if (update_timer) {
        lv_timer_delete(update_timer);
        update_timer = NULL;
    }
    screen = NULL;
}

lv_obj_t *clock_analog_get_screen(void) {
    return screen;
}

void clock_analog_set_sweep_mode(sweep_mode_t mode) {
    if (mode >= SWEEP_MODE_COUNT)
        mode = SWEEP_1HZ;
    current_sweep = mode;
    if (update_timer)
        lv_timer_set_period(update_timer, sweep_interval_ms(mode));
}

void clock_analog_set_face_style(watch_face_id_t id) {
    active_style = watch_face_get_style(id);
    current_sweep = active_style->default_sweep;
}
