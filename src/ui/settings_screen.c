#include "settings_screen.h"
#include "display.h"
#include "display_driver.h"
#include "settings.h"
#include "theme.h"
#include <stdio.h>

#define SLIDER_WIDTH  220
#define DOT_SIZE       36
#define DOT_GAP        12
#define SEG_BTN_W      90
#define SEG_BTN_H      44
#define BACK_BTN_W    120
#define BACK_BTN_H     44

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
            lv_obj_set_style_border_width(theme_dots[i], 2, 0);
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
    lv_obj_t *dot = lv_event_get_target(e);
    uint8_t id = (uint8_t)(uintptr_t)lv_event_get_user_data(e);

    (void)dot;
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

static void back_click_cb(lv_event_t *e) {
    (void)e;
    if (back_cb) back_cb();
}

/* ── public API ──────────────────────────────────────────── */

void settings_screen_create(void) {
    const theme_colors_t *c = theme_get_colors();
    const settings_t *s = settings_get();

    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, c->bg, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    /* ── Title ─────────────────────────────────────────── */
    title_label = lv_label_create(screen);
    lv_label_set_text(title_label, "Settings");
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title_label, c->primary, 0);
    lv_obj_set_style_text_letter_space(title_label, 2, 0);
    lv_obj_align(title_label, LV_ALIGN_CENTER, 0, -180);

    /* ── Theme section ─────────────────────────────────── */
    theme_label = lv_label_create(screen);
    lv_label_set_text(theme_label, "Theme");
    lv_obj_set_style_text_font(theme_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(theme_label, c->secondary, 0);
    lv_obj_align(theme_label, LV_ALIGN_CENTER, 0, -130);

    /*
     * 5 dots centered horizontally:
     *   total width = 5*36 + 4*12 = 228
     *   first dot center X offset = -(228/2) + 18 = -96
     */
    int total_w = THEME_COUNT * DOT_SIZE + (THEME_COUNT - 1) * DOT_GAP;
    int start_x = -(total_w / 2) + (DOT_SIZE / 2);

    for (int i = 0; i < THEME_COUNT; i++) {
        lv_obj_t *dot = lv_obj_create(screen);
        lv_obj_set_size(dot, DOT_SIZE, DOT_SIZE);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(dot, dot_colors[i], 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_style_border_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_style_pad_all(dot, 0, 0);
        lv_obj_remove_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(dot, LV_OBJ_FLAG_CLICKABLE);

        /* Enlarge touch area to 44px min */
        int pad = (44 - DOT_SIZE) / 2;
        if (pad > 0) {
            lv_obj_set_style_pad_all(dot, 0, 0);
            lv_obj_set_ext_click_area(dot, pad);
        }

        int x_off = start_x + i * (DOT_SIZE + DOT_GAP);
        lv_obj_align(dot, LV_ALIGN_CENTER, x_off, -100);

        lv_obj_add_event_cb(dot, dot_click_cb, LV_EVENT_CLICKED,
                            (void *)(uintptr_t)i);
        theme_dots[i] = dot;
    }
    style_dot_selected(s->theme_id);

    /* ── Brightness section ────────────────────────────── */
    bright_label = lv_label_create(screen);
    lv_label_set_text(bright_label, "Brightness");
    lv_obj_set_style_text_font(bright_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(bright_label, c->secondary, 0);
    lv_obj_align(bright_label, LV_ALIGN_CENTER, -20, -40);

    bright_val = lv_label_create(screen);
    {
        char buf[8];
        snprintf(buf, sizeof(buf), "%d%%", s->brightness);
        lv_label_set_text(bright_val, buf);
    }
    lv_obj_set_style_text_font(bright_val, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(bright_val, c->secondary, 0);
    lv_obj_align_to(bright_val, bright_label, LV_ALIGN_OUT_RIGHT_MID, 8, 0);

    bright_slider = lv_slider_create(screen);
    lv_obj_set_width(bright_slider, SLIDER_WIDTH);
    lv_slider_set_range(bright_slider, 0, 100);
    lv_slider_set_value(bright_slider, s->brightness, LV_ANIM_OFF);
    lv_obj_align(bright_slider, LV_ALIGN_CENTER, 0, -8);

    lv_obj_set_style_bg_color(bright_slider, c->accent, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(bright_slider, c->accent, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(bright_slider, LV_OPA_30, LV_PART_MAIN);

    lv_obj_add_event_cb(bright_slider, slider_changed_cb,
                        LV_EVENT_VALUE_CHANGED, NULL);

    /* ── Clock mode section ────────────────────────────── */
    mode_label = lv_label_create(screen);
    lv_label_set_text(mode_label, "Clock Mode");
    lv_obj_set_style_text_font(mode_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(mode_label, c->secondary, 0);
    lv_obj_align(mode_label, LV_ALIGN_CENTER, 0, 50);

    static const char *mode_names[2] = {"Analog", "Digital"};
    for (int i = 0; i < 2; i++) {
        mode_btn[i] = lv_button_create(screen);
        lv_obj_set_size(mode_btn[i], SEG_BTN_W, SEG_BTN_H);
        lv_obj_set_style_radius(mode_btn[i], 6, 0);
        lv_obj_set_style_border_side(mode_btn[i], LV_BORDER_SIDE_FULL, 0);
        lv_obj_set_style_shadow_width(mode_btn[i], 0, 0);
        lv_obj_set_style_pad_all(mode_btn[i], 0, 0);

        int x_off = (i == 0) ? -(SEG_BTN_W / 2 + 2) : (SEG_BTN_W / 2 + 2);
        lv_obj_align(mode_btn[i], LV_ALIGN_CENTER, x_off, 82);

        mode_btn_label[i] = lv_label_create(mode_btn[i]);
        lv_label_set_text(mode_btn_label[i], mode_names[i]);
        lv_obj_set_style_text_font(mode_btn_label[i], &lv_font_montserrat_14, 0);
        lv_obj_center(mode_btn_label[i]);

        lv_obj_add_event_cb(mode_btn[i], mode_click_cb, LV_EVENT_CLICKED,
                            (void *)(uintptr_t)i);
    }
    style_mode_btns(s->clock_mode);

    /* ── Back button ───────────────────────────────────── */
    back_btn = lv_button_create(screen);
    lv_obj_set_size(back_btn, BACK_BTN_W, BACK_BTN_H);
    lv_obj_set_style_radius(back_btn, 6, 0);
    lv_obj_set_style_bg_opa(back_btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(back_btn, c->secondary, 0);
    lv_obj_set_style_border_width(back_btn, 1, 0);
    lv_obj_set_style_shadow_width(back_btn, 0, 0);
    lv_obj_align(back_btn, LV_ALIGN_CENTER, 0, 170);

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
