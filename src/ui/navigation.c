/*
 * Screen navigation via LVGL gesture events
 *
 * LVGL's lv_indev detects swipes/taps from raw touch coordinates.
 * We attach event callbacks to clock screens to route gestures.
 *
 *  ┌──────────┐  swipe L/R  ┌──────────┐
 *  │  Analog  │◄────────────►│ Digital  │
 *  └────┬─────┘              └────┬─────┘
 *       │ tap                     │ tap
 *       ▼                         ▼
 *  ┌──────────┐              ┌──────────┐
 *  │ Settings │──set time──►│ Time Set │
 *  └──────────┘              └──────────┘
 *       │ back_cb                 │ done_cb
 *       └─────────► clock ◄──────┘
 */

#include "navigation.h"
#include "clock_analog.h"
#include "clock_digital.h"
#include "settings_screen.h"
#include "time_set_screen.h"
#include "settings.h"
#include "lvgl.h"

#define ANIM_DURATION_MS 300

typedef enum {
    SCREEN_ANALOG,
    SCREEN_DIGITAL,
    SCREEN_SETTINGS,
    SCREEN_TIME_SET,
} nav_screen_t;

static nav_screen_t current_screen;

static bool transitioning;

static void anim_done_cb(lv_anim_t *a) {
    (void)a;
    transitioning = false;
}

static void start_transition_timer(void) {
    static int32_t dummy;
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, &dummy);
    lv_anim_set_values(&a, 0, 1);
    lv_anim_set_duration(&a, ANIM_DURATION_MS);
    lv_anim_set_completed_cb(&a, anim_done_cb);
    lv_anim_start(&a);
}

static void create_clock_face(nav_screen_t which);

static lv_obj_t *get_clock_screen(nav_screen_t which) {
    if (which == SCREEN_ANALOG)
        return clock_analog_get_screen();
    return clock_digital_get_screen();
}

static nav_screen_t clock_mode_from_settings(void) {
    return (settings_get()->clock_mode == 1) ? SCREEN_DIGITAL : SCREEN_ANALOG;
}

static void switch_clock_mode(lv_scr_load_anim_t anim) {
    if (transitioning) return;
    transitioning = true;

    nav_screen_t target = (current_screen == SCREEN_ANALOG)
                              ? SCREEN_DIGITAL
                              : SCREEN_ANALOG;

    create_clock_face(target);

    lv_screen_load_anim(get_clock_screen(target), anim,
                        ANIM_DURATION_MS, 0, true);

    current_screen = target;

    uint8_t mode = (target == SCREEN_ANALOG) ? 0 : 1;
    settings_set_clock_mode(mode);
    settings_save();

    start_transition_timer();
}

static void clock_gesture_cb(lv_event_t *e) {
    (void)e;
    if (transitioning) return;

    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
    switch (dir) {
    case LV_DIR_LEFT:
        switch_clock_mode(LV_SCR_LOAD_ANIM_MOVE_LEFT);
        break;
    case LV_DIR_RIGHT:
        switch_clock_mode(LV_SCR_LOAD_ANIM_MOVE_RIGHT);
        break;
    default:
        break;
    }
}

static void clock_tap_cb(lv_event_t *e) {
    (void)e;
    nav_show_settings();
}

static void create_clock_face(nav_screen_t which) {
    if (which == SCREEN_ANALOG)
        clock_analog_create();
    else
        clock_digital_create();

    lv_obj_t *scr = get_clock_screen(which);
    lv_obj_add_event_cb(scr, clock_gesture_cb, LV_EVENT_GESTURE, NULL);
    lv_obj_add_event_cb(scr, clock_tap_cb, LV_EVENT_CLICKED, NULL);
}

void nav_init(void) {
    nav_screen_t face = clock_mode_from_settings();
    create_clock_face(face);
    lv_screen_load(get_clock_screen(face));
    current_screen = face;
    transitioning = false;
}

void nav_show_clock(void) {
    if (transitioning) return;
    transitioning = true;

    /* Always respect the current settings for clock mode */
    nav_screen_t face = clock_mode_from_settings();
    current_screen = face;
    create_clock_face(face);
    lv_screen_load_anim(get_clock_screen(face),
                        LV_SCR_LOAD_ANIM_MOVE_BOTTOM,
                        ANIM_DURATION_MS, 0, true);

    start_transition_timer();
}

void nav_show_settings(void) {
    if (transitioning) return;
    transitioning = true;

    settings_screen_create();
    settings_screen_set_back_cb(nav_show_clock);

    lv_screen_load_anim(settings_screen_get_screen(),
                        LV_SCR_LOAD_ANIM_MOVE_TOP,
                        ANIM_DURATION_MS, 0, false);

    current_screen = SCREEN_SETTINGS;

    start_transition_timer();
}

void nav_show_time_set(void) {
    if (transitioning) return;
    transitioning = true;

    time_set_screen_create();
    time_set_screen_set_done_cb(nav_show_clock);

    lv_screen_load_anim(time_set_screen_get_screen(),
                        LV_SCR_LOAD_ANIM_MOVE_TOP,
                        ANIM_DURATION_MS, 0, false);

    current_screen = SCREEN_TIME_SET;

    start_transition_timer();
}
