#include "settings_screen.h"
#include "navigation.h"
#include "display.h"
#include "display_driver.h"
#include "settings.h"
#include "theme.h"
#include <stdio.h>

#define CONTENT_W     360
#define SLIDER_WIDTH  300
#define DOT_SIZE       44
#define DOT_GAP        16
#define BTN_W         140
#define BTN_H          50
#define SEG_BTN_W     140
#define SEG_BTN_H      50
#define SECTION_GAP    24

/*
 * Accent color for each theme — mirrored from theme.c palette.
 * Used to paint the theme-picker dots so the user sees a preview
 * of the accent before selecting.
 */
static const lv_color_t dot_colors[THEME_COUNT] = {
    [THEME_DARK]  = {.red = 0xFF, .green = 0x40, .blue = 0x40},
    [THEME_LIGHT] = {.red = 0xC8, .green = 0xA0, .blue = 0x50},
    [THEME_BLUE]  = {.red = 0x4A, .green = 0x90, .blue = 0xD0},
    [THEME_RED]   = {.red = 0xD0, .green = 0x40, .blue = 0x40},
    [THEME_GREEN] = {.red = 0xC8, .green = 0xA0, .blue = 0x50},
};

static lv_obj_t *screen;
static lv_obj_t *content;
static lv_obj_t *title_label;
static lv_obj_t *theme_label;
static lv_obj_t *theme_dots[THEME_COUNT];
static lv_obj_t *bright_label;
static lv_obj_t *bright_val;
static lv_obj_t *bright_slider;
static lv_obj_t *mode_label;
static lv_obj_t *mode_btn[2];
static lv_obj_t *mode_btn_label[2];
static lv_obj_t *back_btn;

static settings_back_cb_t back_cb;

/* ── helpers ─────────────────────────────────────────────── */

static void style_dot_selected(uint8_t id) {
    const theme_colors_t *c = theme_get_colors();
    for (int i = 0; i < THEME_COUNT; i++) {
        if (i == id) {
            lv_obj_set_style_border_color(theme_dots[i], c->primary, 0);
            lv_obj_set_style_border_width(theme_dots[i], 3, 0);
        } else {
            lv_obj_set_style_border_width(theme_dots[i], 0, 0);
        }
    }
}

static void style_mode_btns(uint8_t mode) {
    const theme_colors_t *c = theme_get_colors();
    for (int i = 0; i < 2; i++) {
        if (i == mode) {
            lv_obj_set_style_bg_color(mode_btn[i], c->accent, 0);
            lv_obj_set_style_bg_opa(mode_btn[i], LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(mode_btn[i], 0, 0);
            lv_obj_set_style_text_color(mode_btn_label[i], c->bg, 0);
        } else {
            lv_obj_set_style_bg_opa(mode_btn[i], LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_color(mode_btn[i], c->secondary, 0);
            lv_obj_set_style_border_width(mode_btn[i], 1, 0);
            lv_obj_set_style_text_color(mode_btn_label[i], c->primary, 0);
        }
    }
}

static void apply_colors(void) {
    const theme_colors_t *c = theme_get_colors();

    lv_obj_set_style_bg_color(screen, c->bg, 0);
    lv_obj_set_style_text_color(title_label, c->primary, 0);
    lv_obj_set_style_text_color(theme_label, c->secondary, 0);
    lv_obj_set_style_text_color(bright_label, c->secondary, 0);
    lv_obj_set_style_text_color(bright_val, c->secondary, 0);
    lv_obj_set_style_text_color(mode_label, c->secondary, 0);

    lv_obj_set_style_bg_color(bright_slider, c->accent, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(bright_slider, c->accent, LV_PART_KNOB);

    lv_obj_set_style_border_color(back_btn, c->secondary, 0);
    lv_obj_set_style_text_color(lv_obj_get_child(back_btn, 0), c->primary, 0);

    style_dot_selected(theme_get_current_id());
    style_mode_btns(settings_get()->clock_mode);
}

/* ── theme change callback ───────────────────────────────── */

static void on_theme_changed(const theme_colors_t *colors) {
    (void)colors;
    if (!screen) return;
    apply_colors();
}

/* ── event handlers ──────────────────────────────────────── */

static void dot_click_cb(lv_event_t *e) {
    uint8_t id = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    theme_apply(id);
    settings_set_theme_id(id);
    settings_save();
    style_dot_selected(id);
}

static void slider_changed_cb(lv_event_t *e) {
    (void)e;
    int32_t val = lv_slider_get_value(bright_slider);
    display_set_brightness((uint8_t)val);
    settings_set_brightness((uint8_t)val);
    settings_save();

    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", (int)val);
    lv_label_set_text(bright_val, buf);
}

static void mode_click_cb(lv_event_t *e) {
    uint8_t mode = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    settings_set_clock_mode(mode);
    settings_save();
    style_mode_btns(mode);
}

static void time_set_click_cb(lv_event_t *e) {
    (void)e;
    nav_show_time_set();
}

static void back_click_cb(lv_event_t *e) {
    (void)e;
    if (back_cb) back_cb();
}

/* ── section label helper ────────────────────────────────── */

static lv_obj_t *add_section_label(lv_obj_t *parent, const char *text) {
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl, theme_get_colors()->secondary, 0);
    lv_obj_set_width(lbl, CONTENT_W);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    return lbl;
}

/* ── public API ──────────────────────────────────────────── */

void settings_screen_create(void) {
    const theme_colors_t *c = theme_get_colors();
    const settings_t *s = settings_get();

    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, c->bg, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    /*
     * Scrollable content column — centered in screen.
     * Flex column layout so items flow top-to-bottom and
     * the whole thing scrolls vertically.
     */
    content = lv_obj_create(screen);
    lv_obj_set_size(content, CONTENT_W, DISPLAY_HEIGHT);
    lv_obj_align(content, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_style_flex_flow(content, LV_FLEX_FLOW_COLUMN, 0);
    lv_obj_set_style_flex_main_place(content, LV_FLEX_ALIGN_START, 0);
    lv_obj_set_style_flex_cross_place(content, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_top(content, 60, 0);
    lv_obj_set_style_pad_bottom(content, 80, 0);
    lv_obj_set_style_pad_left(content, 0, 0);
    lv_obj_set_style_pad_right(content, 12, 0);  /* room for scrollbar */
    lv_obj_set_style_pad_row(content, 12, 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(content, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_OFF);

    /* ── Title ─────────────────────────────────────────── */
    title_label = lv_label_create(content);
    lv_label_set_text(title_label, "Settings");
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title_label, c->primary, 0);
    lv_obj_set_style_text_letter_space(title_label, 2, 0);
    lv_obj_set_style_pad_bottom(title_label, SECTION_GAP, 0);

    /* ── Theme section ─────────────────────────────────── */
    theme_label = add_section_label(content, "Theme");

    lv_obj_t *dot_row = lv_obj_create(content);
    lv_obj_set_size(dot_row, CONTENT_W, LV_SIZE_CONTENT);
    lv_obj_set_layout(dot_row, LV_LAYOUT_FLEX);
    lv_obj_set_style_flex_flow(dot_row, LV_FLEX_FLOW_ROW, 0);
    lv_obj_set_style_flex_main_place(dot_row, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_column(dot_row, DOT_GAP, 0);
    lv_obj_set_style_pad_all(dot_row, 0, 0);
    lv_obj_set_style_bg_opa(dot_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(dot_row, 0, 0);
    lv_obj_remove_flag(dot_row, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < THEME_COUNT; i++) {
        lv_obj_t *dot = lv_obj_create(dot_row);
        lv_obj_set_size(dot, DOT_SIZE, DOT_SIZE);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(dot, dot_colors[i], 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_style_border_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_style_pad_all(dot, 0, 0);
        lv_obj_remove_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(dot, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_ext_click_area(dot, 8);

        lv_obj_add_event_cb(dot, dot_click_cb, LV_EVENT_CLICKED,
                            (void *)(uintptr_t)i);
        theme_dots[i] = dot;
    }
    style_dot_selected(s->theme_id);

    /* ── Spacer ───────────────────────────────────────── */
    lv_obj_t *sp1 = lv_obj_create(content);
    lv_obj_set_size(sp1, 1, SECTION_GAP);
    lv_obj_set_style_bg_opa(sp1, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(sp1, 0, 0);

    /* ── Brightness section ────────────────────────────── */
    lv_obj_t *bright_row = lv_obj_create(content);
    lv_obj_set_size(bright_row, CONTENT_W, LV_SIZE_CONTENT);
    lv_obj_set_layout(bright_row, LV_LAYOUT_FLEX);
    lv_obj_set_style_flex_flow(bright_row, LV_FLEX_FLOW_ROW, 0);
    lv_obj_set_style_flex_main_place(bright_row, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_column(bright_row, 8, 0);
    lv_obj_set_style_pad_all(bright_row, 0, 0);
    lv_obj_set_style_bg_opa(bright_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bright_row, 0, 0);
    lv_obj_remove_flag(bright_row, LV_OBJ_FLAG_SCROLLABLE);

    bright_label = lv_label_create(bright_row);
    lv_label_set_text(bright_label, "Brightness");
    lv_obj_set_style_text_font(bright_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(bright_label, c->secondary, 0);

    bright_val = lv_label_create(bright_row);
    {
        char buf[8];
        snprintf(buf, sizeof(buf), "%d%%", s->brightness);
        lv_label_set_text(bright_val, buf);
    }
    lv_obj_set_style_text_font(bright_val, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(bright_val, c->secondary, 0);

    bright_slider = lv_slider_create(content);
    lv_obj_set_width(bright_slider, SLIDER_WIDTH);
    lv_obj_set_style_min_height(bright_slider, 20, 0);
    lv_slider_set_range(bright_slider, 0, 100);
    lv_slider_set_value(bright_slider, s->brightness, LV_ANIM_OFF);

    lv_obj_set_style_bg_color(bright_slider, c->accent, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(bright_slider, c->accent, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(bright_slider, LV_OPA_30, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(bright_slider, 12, LV_PART_KNOB);
    lv_obj_set_style_pad_hor(bright_slider, 12, LV_PART_KNOB);

    lv_obj_add_event_cb(bright_slider, slider_changed_cb,
                        LV_EVENT_VALUE_CHANGED, NULL);

    /* ── Spacer ───────────────────────────────────────── */
    lv_obj_t *sp2 = lv_obj_create(content);
    lv_obj_set_size(sp2, 1, SECTION_GAP);
    lv_obj_set_style_bg_opa(sp2, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(sp2, 0, 0);

    /* ── Clock mode section ────────────────────────────── */
    mode_label = add_section_label(content, "Clock Mode");

    lv_obj_t *mode_row = lv_obj_create(content);
    lv_obj_set_size(mode_row, CONTENT_W, LV_SIZE_CONTENT);
    lv_obj_set_layout(mode_row, LV_LAYOUT_FLEX);
    lv_obj_set_style_flex_flow(mode_row, LV_FLEX_FLOW_ROW, 0);
    lv_obj_set_style_flex_main_place(mode_row, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_column(mode_row, 12, 0);
    lv_obj_set_style_pad_all(mode_row, 0, 0);
    lv_obj_set_style_bg_opa(mode_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(mode_row, 0, 0);
    lv_obj_remove_flag(mode_row, LV_OBJ_FLAG_SCROLLABLE);

    static const char *mode_names[2] = {"Analog", "Digital"};
    for (int i = 0; i < 2; i++) {
        mode_btn[i] = lv_button_create(mode_row);
        lv_obj_set_size(mode_btn[i], SEG_BTN_W, SEG_BTN_H);
        lv_obj_set_style_radius(mode_btn[i], 6, 0);
        lv_obj_set_style_border_side(mode_btn[i], LV_BORDER_SIDE_FULL, 0);
        lv_obj_set_style_shadow_width(mode_btn[i], 0, 0);
        lv_obj_set_style_pad_all(mode_btn[i], 0, 0);

        mode_btn_label[i] = lv_label_create(mode_btn[i]);
        lv_label_set_text(mode_btn_label[i], mode_names[i]);
        lv_obj_set_style_text_font(mode_btn_label[i], &lv_font_montserrat_14, 0);
        lv_obj_center(mode_btn_label[i]);

        lv_obj_add_event_cb(mode_btn[i], mode_click_cb, LV_EVENT_CLICKED,
                            (void *)(uintptr_t)i);
    }
    style_mode_btns(s->clock_mode);

    /* ── Spacer ───────────────────────────────────────── */
    lv_obj_t *sp3 = lv_obj_create(content);
    lv_obj_set_size(sp3, 1, SECTION_GAP);
    lv_obj_set_style_bg_opa(sp3, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(sp3, 0, 0);

    /* ── Set Time button ───────────────────────────────── */
    lv_obj_t *time_btn = lv_button_create(content);
    lv_obj_set_size(time_btn, BTN_W, BTN_H);
    lv_obj_set_style_radius(time_btn, 6, 0);
    lv_obj_set_style_bg_color(time_btn, c->accent, 0);
    lv_obj_set_style_bg_opa(time_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_width(time_btn, 0, 0);

    lv_obj_t *time_lbl = lv_label_create(time_btn);
    lv_label_set_text(time_lbl, "Set Time");
    lv_obj_set_style_text_font(time_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(time_lbl, c->bg, 0);
    lv_obj_center(time_lbl);

    lv_obj_add_event_cb(time_btn, time_set_click_cb, LV_EVENT_CLICKED, NULL);

    /* ── Back button ───────────────────────────────────── */
    back_btn = lv_button_create(content);
    lv_obj_set_size(back_btn, BTN_W, BTN_H);
    lv_obj_set_style_radius(back_btn, 6, 0);
    lv_obj_set_style_bg_opa(back_btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(back_btn, c->secondary, 0);
    lv_obj_set_style_border_width(back_btn, 1, 0);
    lv_obj_set_style_shadow_width(back_btn, 0, 0);

    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, "Back");
    lv_obj_set_style_text_font(back_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(back_lbl, c->primary, 0);
    lv_obj_center(back_lbl);

    lv_obj_add_event_cb(back_btn, back_click_cb, LV_EVENT_CLICKED, NULL);

    theme_register_callback(on_theme_changed);
}

void settings_screen_destroy(void) {
    theme_unregister_callback(on_theme_changed);
    if (screen) {
        lv_obj_delete(screen);
        screen = NULL;
    }
}

lv_obj_t *settings_screen_get_screen(void) {
    return screen;
}

void settings_screen_set_back_cb(settings_back_cb_t cb) {
    back_cb = cb;
}
