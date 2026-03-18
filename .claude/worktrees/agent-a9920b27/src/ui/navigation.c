/*
 * Screen navigation and gesture routing
 *
 * Manages transitions between clock faces (analog/digital) and the
 * settings screen. Gesture source is touch_read() polled in main loop.
 *
 *  ┌──────────┐  swipe L/R  ┌──────────┐
 *  │  Analog  │◄────────────►│ Digital  │
 *  └────┬─────┘              └────┬─────┘
 *       │ tap                     │ tap
 *       ▼                         ▼
 *  ┌──────────┐              ┌──────────┐
 *  │ Settings │              │ Settings │
 *  └──────────┘              └──────────┘
 *       │ back_cb                 │ back_cb
 *       └─────────► clock ◄──────┘
 */

#include "navigation.h"
#include "clock_analog.h"
#include "clock_digital.h"
#include "settings_screen.h"
#include "settings.h"
#include "lvgl.h"

#define ANIM_DURATION_MS 300

typedef enum {
    SCREEN_ANALOG,
    SCREEN_DIGITAL,
    SCREEN_SETTINGS,
} nav_screen_t;

static nav_screen_t current_screen;

/* Debounce: ignore gestures while a transition is in flight */
static bool transitioning;

static void anim_done_cb(lv_anim_t *a) {
    (void)a;
    transitioning = false;
}

static void create_clock_face(nav_screen_t which) {
    if (which == SCREEN_ANALOG) {
        clock_analog_create();
    } else {
        clock_digital_create();
    }
}

static lv_obj_t *get_clock_screen(nav_screen_t which) {
    if (which == SCREEN_ANALOG)
        return clock_analog_get_screen();
    return clock_digital_get_screen();
}

static void switch_clock_mode(lv_scr_load_anim_t anim) {
    if (transitioning) return;
    transitioning = true;

    nav_screen_t target = (current_screen == SCREEN_ANALOG)
                              ? SCREEN_DIGITAL
                              : SCREEN_ANALOG;

    create_clock_face(target);

    /* auto_del=true tells LVGL to delete the old screen after animation */
    lv_screen_load_anim(get_clock_screen(target), anim,
                        ANIM_DURATION_MS, 0, true);

    current_screen = target;

    uint8_t mode = (target == SCREEN_ANALOG) ? 0 : 1;
    settings_set_clock_mode(mode);
    settings_save();

    /*
     * lv_screen_load_anim doesn't expose a completion callback directly.
     * Use a one-shot LVGL animation on a dummy value to clear the flag
     * after the same duration.
     */
    static int32_t dummy;
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, &dummy);
    lv_anim_set_values(&a, 0, 1);
    lv_anim_set_duration(&a, ANIM_DURATION_MS);
    lv_anim_set_completed_cb(&a, anim_done_cb);
    lv_anim_start(&a);
}

void nav_init(void) {
    const settings_t *s = settings_get();
    if (s->clock_mode == 1) {
        create_clock_face(SCREEN_DIGITAL);
        lv_screen_load(clock_digital_get_screen());
        current_screen = SCREEN_DIGITAL;
    } else {
        create_clock_face(SCREEN_ANALOG);
        lv_screen_load(clock_analog_get_screen());
        current_screen = SCREEN_ANALOG;
    }
    transitioning = false;
}

void nav_handle_gesture(const touch_event_t *event) {
    if (transitioning) return;
    if (event->gesture == GESTURE_NONE) return;

    /* Only route gestures when on a clock screen */
    if (current_screen == SCREEN_SETTINGS) return;

    switch (event->gesture) {
    case GESTURE_SWIPE_LEFT:
        switch_clock_mode(LV_SCR_LOAD_ANIM_MOVE_LEFT);
        break;
    case GESTURE_SWIPE_RIGHT:
        switch_clock_mode(LV_SCR_LOAD_ANIM_MOVE_RIGHT);
        break;
    case GESTURE_TAP:
        nav_show_settings();
        break;
    default:
        break;
    }
}

void nav_show_clock(void) {
    if (transitioning) return;
    transitioning = true;

    create_clock_face(current_screen);
    lv_screen_load_anim(get_clock_screen(current_screen),
                        LV_SCR_LOAD_ANIM_MOVE_BOTTOM,
                        ANIM_DURATION_MS, 0, true);

    static int32_t dummy;
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, &dummy);
    lv_anim_set_values(&a, 0, 1);
    lv_anim_set_duration(&a, ANIM_DURATION_MS);
    lv_anim_set_completed_cb(&a, anim_done_cb);
    lv_anim_start(&a);
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

    static int32_t dummy;
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, &dummy);
    lv_anim_set_values(&a, 0, 1);
    lv_anim_set_duration(&a, ANIM_DURATION_MS);
    lv_anim_set_completed_cb(&a, anim_done_cb);
    lv_anim_start(&a);
}
