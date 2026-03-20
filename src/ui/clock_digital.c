#include "clock_digital.h"
#include "display.h"
#include "rtc_driver.h"
#include "theme.h"
#include <stdio.h>

#define CENTER_X (DISPLAY_WIDTH / 2)
#define CENTER_Y (DISPLAY_HEIGHT / 2)

static const char *month_abbr[] = {
    "", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static const char *weekday_abbr[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static lv_obj_t *screen;
static lv_obj_t *time_label;
static lv_obj_t *seconds_label;
static lv_obj_t *sep_line;
static lv_obj_t *date_label;
static lv_obj_t *year_label;

static lv_point_precise_t sep_pts[2];
static int8_t last_second = -1;
static lv_timer_t *update_timer;

static void timer_cb(lv_timer_t *timer) {
    (void)timer;
    clock_digital_update();
}

void clock_digital_create(void) {
    const theme_colors_t *c = theme_get_colors();

    if (update_timer) {
        lv_timer_delete(update_timer);
        update_timer = NULL;
    }
    last_second = -1;

    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, c->bg, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    /* HH:MM — large, slightly above center */
    time_label = lv_label_create(screen);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(time_label, c->primary, 0);
    lv_obj_align(time_label, LV_ALIGN_CENTER, 0, -40);

    /* :SS — accent colored to match analog second hand */
    seconds_label = lv_label_create(screen);
    lv_obj_set_style_text_font(seconds_label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(seconds_label, c->accent, 0);
    lv_obj_set_style_text_opa(seconds_label, LV_OPA_80, 0);
    lv_obj_align_to(seconds_label, time_label, LV_ALIGN_OUT_RIGHT_BOTTOM, 4, -2);

    /* Separator line in accent */
    sep_pts[0] = (lv_point_precise_t){CENTER_X - 60, CENTER_Y + 16};
    sep_pts[1] = (lv_point_precise_t){CENTER_X + 60, CENTER_Y + 16};
    sep_line = lv_line_create(screen);
    lv_line_set_points(sep_line, sep_pts, 2);
    lv_obj_set_style_line_color(sep_line, c->accent, 0);
    lv_obj_set_style_line_opa(sep_line, LV_OPA_40, 0);
    lv_obj_set_style_line_width(sep_line, 1, 0);

    /* Date: "Wed, Mar 18" */
    date_label = lv_label_create(screen);
    lv_obj_set_style_text_font(date_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(date_label, c->secondary, 0);
    lv_obj_set_style_text_letter_space(date_label, 2, 0);
    lv_obj_align(date_label, LV_ALIGN_CENTER, 0, 48);

    /* Year */
    year_label = lv_label_create(screen);
    lv_obj_set_style_text_font(year_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(year_label, c->secondary, 0);
    lv_obj_set_style_text_opa(year_label, LV_OPA_60, 0);
    lv_obj_set_style_text_letter_space(year_label, 4, 0);
    lv_obj_align(year_label, LV_ALIGN_CENTER, 0, 76);

    clock_digital_update();

    update_timer = lv_timer_create(timer_cb, 1000, NULL);
}

void clock_digital_update(void) {
    rtc_datetime_t dt;
    rtc_app_get_datetime(&dt);

    if (dt.second == last_second)
        return;
    last_second = dt.second;

    char buf[32];

    snprintf(buf, sizeof(buf), "%02u:%02u", dt.hour, dt.minute);
    lv_label_set_text(time_label, buf);

    snprintf(buf, sizeof(buf), ":%02u", dt.second);
    lv_label_set_text(seconds_label, buf);
    /* Re-align seconds label after time text may have changed width */
    lv_obj_align_to(seconds_label, time_label, LV_ALIGN_OUT_RIGHT_BOTTOM, 4, -2);

    snprintf(buf, sizeof(buf), "%s, %s %u",
             weekday_abbr[dt.dotw % 7],
             month_abbr[dt.month],
             dt.day);
    lv_label_set_text(date_label, buf);

    snprintf(buf, sizeof(buf), "%u", dt.year);
    lv_label_set_text(year_label, buf);
}

void clock_digital_destroy(void) {
    if (update_timer) {
        lv_timer_delete(update_timer);
        update_timer = NULL;
    }
    screen = NULL;
}

lv_obj_t *clock_digital_get_screen(void) {
    return screen;
}
