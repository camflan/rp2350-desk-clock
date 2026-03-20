#include "time_set_screen.h"
#include "rtc_driver.h"
#include "settings.h"
#include "theme.h"
#include <stdio.h>

#define CONTENT_W       360
#define ROLLER_W         80
#define ROLLER_W_YEAR   100
#define ROLLER_VIS_ROWS   5
#define BTN_W           140
#define BTN_H            50

#define YEAR_MIN 2024
#define YEAR_MAX 2036

static lv_obj_t *screen;
static lv_obj_t *title_label;
static lv_obj_t *time_label;
static lv_obj_t *date_label;
static lv_obj_t *roller_hour;
static lv_obj_t *roller_min;
static lv_obj_t *roller_sec;
static lv_obj_t *roller_year;
static lv_obj_t *roller_month;
static lv_obj_t *roller_day;
static lv_obj_t *set_btn;
static lv_obj_t *cancel_btn;

static time_set_done_cb_t done_cb;

/* ── option string builders ──────────────────────────────── */

static char *build_num_options(char *buf, size_t len, int min, int max, int width) {
    size_t pos = 0;
    for (int i = min; i <= max; i++) {
        if (i > min) buf[pos++] = '\n';
        int written = snprintf(buf + pos, len - pos, "%0*d", width, i);
        if (written < 0) break;
        pos += (size_t)written;
        if (pos >= len) break;
    }
    return buf;
}

/* ── theme ───────────────────────────────────────────────── */

static void apply_colors(void) {
    const theme_colors_t *c = theme_get_colors();

    lv_obj_set_style_bg_color(screen, c->bg, 0);
    lv_obj_set_style_text_color(title_label, c->primary, 0);
    lv_obj_set_style_text_color(time_label, c->secondary, 0);
    lv_obj_set_style_text_color(date_label, c->secondary, 0);

    lv_obj_t *rollers[] = {
        roller_hour, roller_min, roller_sec,
        roller_year, roller_month, roller_day
    };
    for (int i = 0; i < 6; i++) {
        lv_obj_set_style_text_color(rollers[i], c->secondary, 0);
        lv_obj_set_style_text_color(rollers[i], c->primary, LV_PART_SELECTED);
        lv_obj_set_style_bg_color(rollers[i], c->accent, LV_PART_SELECTED);
        lv_obj_set_style_bg_opa(rollers[i], LV_OPA_30, LV_PART_SELECTED);
    }

    lv_obj_set_style_bg_color(set_btn, c->accent, 0);
    lv_obj_set_style_bg_opa(set_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(lv_obj_get_child(set_btn, 0), c->bg, 0);

    lv_obj_set_style_bg_opa(cancel_btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(cancel_btn, c->secondary, 0);
    lv_obj_set_style_border_width(cancel_btn, 1, 0);
    lv_obj_set_style_text_color(lv_obj_get_child(cancel_btn, 0), c->primary, 0);
}

static void on_theme_changed(const theme_colors_t *colors) {
    (void)colors;
    if (!screen) return;
    apply_colors();
}

/* ── event handlers ──────────────────────────────────────── */

static void set_click_cb(lv_event_t *e) {
    (void)e;

    rtc_datetime_t dt = {0};
    dt.hour   = (uint8_t)lv_roller_get_selected(roller_hour);
    dt.minute = (uint8_t)lv_roller_get_selected(roller_min);
    dt.second = (uint8_t)lv_roller_get_selected(roller_sec);
    dt.year   = (uint16_t)(YEAR_MIN + lv_roller_get_selected(roller_year));
    dt.month  = (uint8_t)(1 + lv_roller_get_selected(roller_month));
    dt.day    = (uint8_t)(1 + lv_roller_get_selected(roller_day));

    rtc_app_set_datetime(&dt);
    settings_set_last_datetime(&dt);
    settings_save();

    if (done_cb) done_cb();
}

static void cancel_click_cb(lv_event_t *e) {
    (void)e;
    if (done_cb) done_cb();
}

/* ── helpers ─────────────────────────────────────────────── */

static lv_obj_t *create_roller(lv_obj_t *parent, const char *opts, int width) {
    lv_obj_t *r = lv_roller_create(parent);
    lv_roller_set_options(r, opts, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(r, ROLLER_VIS_ROWS);
    lv_obj_set_width(r, width);
    lv_obj_set_style_text_font(r, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_font(r, &lv_font_montserrat_28, LV_PART_SELECTED);
    lv_obj_set_style_border_width(r, 0, 0);
    lv_obj_set_style_bg_opa(r, LV_OPA_TRANSP, 0);
    lv_obj_set_style_text_align(r, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_anim_duration(r, 200, 0);
    return r;
}

static lv_obj_t *create_flex_row(lv_obj_t *parent, int gap) {
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, CONTENT_W, LV_SIZE_CONTENT);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_style_flex_flow(row, LV_FLEX_FLOW_ROW, 0);
    lv_obj_set_style_flex_main_place(row, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_flex_cross_place(row, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_column(row, gap, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    return row;
}

/* ── public API ──────────────────────────────────────────── */

void time_set_screen_create(void) {
    const theme_colors_t *c = theme_get_colors();

    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, c->bg, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    /* Flex column container */
    lv_obj_t *cont = lv_obj_create(screen);
    lv_obj_set_size(cont, CONTENT_W, LV_SIZE_CONTENT);
    lv_obj_align(cont, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
    lv_obj_set_style_flex_flow(cont, LV_FLEX_FLOW_COLUMN, 0);
    lv_obj_set_style_flex_main_place(cont, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_flex_cross_place(cont, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_row(cont, 8, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    /* ── Title ─────────────────────────────────────────── */
    title_label = lv_label_create(cont);
    lv_label_set_text(title_label, "Set Time");
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title_label, c->primary, 0);
    lv_obj_set_style_text_letter_space(title_label, 2, 0);
    lv_obj_set_style_pad_bottom(title_label, 8, 0);

    /* ── Option strings ────────────────────────────────── */
    static char hour_opts[24 * 3];
    static char minsec_opts[60 * 3];
    static char year_opts[(YEAR_MAX - YEAR_MIN + 1) * 5];
    static char month_opts[12 * 3];
    static char day_opts[31 * 3];

    build_num_options(hour_opts,   sizeof(hour_opts),   0, 23, 2);
    build_num_options(minsec_opts, sizeof(minsec_opts), 0, 59, 2);
    build_num_options(year_opts,   sizeof(year_opts),   YEAR_MIN, YEAR_MAX, 4);
    build_num_options(month_opts,  sizeof(month_opts),  1, 12, 2);
    build_num_options(day_opts,    sizeof(day_opts),    1, 31, 2);

    /* ── "Time" label ──────────────────────────────────── */
    time_label = lv_label_create(cont);
    lv_label_set_text(time_label, "Time");
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(time_label, c->secondary, 0);

    /* ── Time rollers: [HH] [MM] [SS] ──────────────────── */
    lv_obj_t *time_row = create_flex_row(cont, 8);

    roller_hour = create_roller(time_row, hour_opts, ROLLER_W);
    roller_min  = create_roller(time_row, minsec_opts, ROLLER_W);
    roller_sec  = create_roller(time_row, minsec_opts, ROLLER_W);

    /* ── "Date" label ──────────────────────────────────── */
    date_label = lv_label_create(cont);
    lv_label_set_text(date_label, "Date");
    lv_obj_set_style_text_font(date_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(date_label, c->secondary, 0);
    lv_obj_set_style_pad_top(date_label, 4, 0);

    /* ── Date rollers: [YYYY] [MM] [DD] ────────────────── */
    lv_obj_t *date_row = create_flex_row(cont, 8);

    roller_year  = create_roller(date_row, year_opts, ROLLER_W_YEAR);
    roller_month = create_roller(date_row, month_opts, ROLLER_W);
    roller_day   = create_roller(date_row, day_opts, ROLLER_W);

    /* ── Initialize rollers to current RTC time ────────── */
    rtc_datetime_t now;
    rtc_app_get_datetime(&now);

    lv_roller_set_selected(roller_hour, now.hour, LV_ANIM_OFF);
    lv_roller_set_selected(roller_min,  now.minute, LV_ANIM_OFF);
    lv_roller_set_selected(roller_sec,  now.second, LV_ANIM_OFF);

    uint16_t yr = now.year;
    if (yr < YEAR_MIN) yr = YEAR_MIN;
    if (yr > YEAR_MAX) yr = YEAR_MAX;
    lv_roller_set_selected(roller_year,  yr - YEAR_MIN, LV_ANIM_OFF);
    lv_roller_set_selected(roller_month, now.month > 0 ? now.month - 1 : 0, LV_ANIM_OFF);
    lv_roller_set_selected(roller_day,   now.day > 0 ? now.day - 1 : 0, LV_ANIM_OFF);

    /* ── Buttons row ──────────────────────────────────── */
    lv_obj_t *btn_row = create_flex_row(cont, 16);
    lv_obj_set_style_pad_top(btn_row, 8, 0);

    cancel_btn = lv_button_create(btn_row);
    lv_obj_set_size(cancel_btn, BTN_W, BTN_H);
    lv_obj_set_style_radius(cancel_btn, 6, 0);
    lv_obj_set_style_shadow_width(cancel_btn, 0, 0);

    lv_obj_t *cancel_lbl = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_lbl, "Cancel");
    lv_obj_set_style_text_font(cancel_lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(cancel_lbl);

    lv_obj_add_event_cb(cancel_btn, cancel_click_cb, LV_EVENT_CLICKED, NULL);

    set_btn = lv_button_create(btn_row);
    lv_obj_set_size(set_btn, BTN_W, BTN_H);
    lv_obj_set_style_radius(set_btn, 6, 0);
    lv_obj_set_style_shadow_width(set_btn, 0, 0);

    lv_obj_t *set_lbl = lv_label_create(set_btn);
    lv_label_set_text(set_lbl, "Set");
    lv_obj_set_style_text_font(set_lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(set_lbl);

    lv_obj_add_event_cb(set_btn, set_click_cb, LV_EVENT_CLICKED, NULL);

    /* ── Apply theme colors ────────────────────────────── */
    apply_colors();
    theme_register_callback(on_theme_changed);
}

void time_set_screen_destroy(void) {
    theme_unregister_callback(on_theme_changed);
    if (screen) {
        lv_obj_delete(screen);
        screen = NULL;
    }
}

lv_obj_t *time_set_screen_get_screen(void) {
    return screen;
}

void time_set_screen_set_done_cb(time_set_done_cb_t cb) {
    done_cb = cb;
}
