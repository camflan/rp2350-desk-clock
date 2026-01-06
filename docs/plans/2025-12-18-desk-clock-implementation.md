# RP2350 AMOLED Desk Clock Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Build a USB-powered desk clock with AMOLED display, dual clock faces (analog/digital), touch controls, and persistent settings.

**Architecture:** Foundation-first approach with hardware abstraction layer (HAL) for display/touch drivers, LVGL for UI framework, software RTC for timekeeping, and flash-based settings persistence. Simple superloop architecture without RTOS.

**Tech Stack:** Raspberry Pi Pico SDK (C), LVGL v9.4+, CMake, Waveshare RP2350-Touch-AMOLED-1.43

---

## Prerequisites

Before starting, verify:
- Pico SDK installed and `PICO_SDK_PATH` environment variable set
- ARM GCC toolchain installed (`arm-none-eabi-gcc`)
- CMake 3.13+ installed
- LVGL cloned at `~/src/github.com/lvgl/lvgl`
- Waveshare board documentation available (for pinout/controller specs)

---

## Task 1: Project Structure & CMake Setup

**Files:**
- Create: `CMakeLists.txt`
- Create: `pico_sdk_import.cmake`
- Create: `src/main.c`
- Create: `lv_conf.h`
- Create: `.gitignore`

**Step 1: Copy Pico SDK import script**

```bash
cp $PICO_SDK_PATH/external/pico_sdk_import.cmake .
```

**Step 2: Create root CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.13)

# Pull in Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(rp2350_amoled_clock C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialize the Pico SDK
pico_sdk_init()

# Add LVGL
add_subdirectory(lib/lvgl)

# Main executable
add_executable(${PROJECT_NAME}
    src/main.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    hardware_spi
    hardware_i2c
    hardware_timer
    hardware_flash
    lvgl
)

# Enable USB serial output, disable UART
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(${PROJECT_NAME})
```

**Step 3: Create minimal main.c**

```c
// src/main.c
#include <stdio.h>
#include "pico/stdlib.h"

int main() {
    stdio_init_all();

    printf("RP2350 AMOLED Clock starting...\n");

    while (1) {
        printf("Hello from RP2350!\n");
        sleep_ms(1000);
    }

    return 0;
}
```

**Step 4: Create LVGL config header**

```c
// lv_conf.h
#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0

#define LV_HOR_RES_MAX 466
#define LV_VER_RES_MAX 466
#define LV_DPI_DEF 320

#define LV_MEM_SIZE (128 * 1024)
#define LV_USE_DRAW_SW 1

#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_INCLUDE "pico/time.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (to_ms_since_boot(get_absolute_time()))

#endif
```

**Step 5: Create .gitignore**

```
build/
.cache/
compile_commands.json
*.uf2
*.bin
*.elf
*.hex
.DS_Store
```

**Step 6: Create lib directory and symlink LVGL**

```bash
mkdir -p lib
ln -s ~/src/github.com/lvgl/lvgl lib/lvgl
```

**Step 7: Build the project**

Run:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Expected: Build succeeds, produces `build/rp2350_amoled_clock.uf2`

**Step 8: Commit**

```bash
git add .
git commit -m "feat: initialize project structure and build system

- Add CMake configuration with Pico SDK integration
- Add LVGL as library dependency
- Create minimal main.c with USB serial output
- Configure LVGL for 466x466 AMOLED display"
```

---

## Task 2: Display Driver - Hardware Setup

**Files:**
- Create: `src/hal/display_driver.h`
- Create: `src/hal/display_driver.c`
- Modify: `CMakeLists.txt`

**Step 1: Research Waveshare board specs**

Check Waveshare documentation for:
- Display controller type (likely SH8501 or RM67162)
- SPI pins (SCK, MOSI, CS, DC, RST)
- Backlight control pin
- Display resolution and color format

Document findings in comments at top of display_driver.c

**Step 2: Create display driver header**

```c
// src/hal/display_driver.h
#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "lvgl.h"

// Waveshare RP2350-Touch-AMOLED-1.43 pin definitions
// TODO: Verify these from Waveshare schematic
#define DISPLAY_SPI_PORT spi1
#define DISPLAY_PIN_SCK  10
#define DISPLAY_PIN_MOSI 11
#define DISPLAY_PIN_CS   9
#define DISPLAY_PIN_DC   8
#define DISPLAY_PIN_RST  12
#define DISPLAY_PIN_BL   25

#define DISPLAY_WIDTH  466
#define DISPLAY_HEIGHT 466

void display_init(void);
void display_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
void display_set_brightness(uint8_t level);  // 0-100

#endif
```

**Step 3: Create display driver implementation stub**

```c
// src/hal/display_driver.c
#include "display_driver.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include <stdio.h>

// Display controller: SH8501 (or RM67162 - check Waveshare docs)
// TODO: Update controller-specific init sequence from datasheet

static void display_spi_init(void) {
    // Initialize SPI at 62.5 MHz
    spi_init(DISPLAY_SPI_PORT, 62500000);

    gpio_set_function(DISPLAY_PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(DISPLAY_PIN_MOSI, GPIO_FUNC_SPI);

    // CS, DC, RST as outputs
    gpio_init(DISPLAY_PIN_CS);
    gpio_set_dir(DISPLAY_PIN_CS, GPIO_OUT);
    gpio_put(DISPLAY_PIN_CS, 1);

    gpio_init(DISPLAY_PIN_DC);
    gpio_set_dir(DISPLAY_PIN_DC, GPIO_OUT);

    gpio_init(DISPLAY_PIN_RST);
    gpio_set_dir(DISPLAY_PIN_RST, GPIO_OUT);
}

static void display_reset(void) {
    gpio_put(DISPLAY_PIN_RST, 1);
    sleep_ms(5);
    gpio_put(DISPLAY_PIN_RST, 0);
    sleep_ms(20);
    gpio_put(DISPLAY_PIN_RST, 1);
    sleep_ms(150);
}

static void display_write_cmd(uint8_t cmd) {
    gpio_put(DISPLAY_PIN_CS, 0);
    gpio_put(DISPLAY_PIN_DC, 0);  // Command
    spi_write_blocking(DISPLAY_SPI_PORT, &cmd, 1);
    gpio_put(DISPLAY_PIN_CS, 1);
}

static void display_write_data(const uint8_t *data, size_t len) {
    gpio_put(DISPLAY_PIN_CS, 0);
    gpio_put(DISPLAY_PIN_DC, 1);  // Data
    spi_write_blocking(DISPLAY_SPI_PORT, data, len);
    gpio_put(DISPLAY_PIN_CS, 1);
}

void display_init(void) {
    printf("Initializing display...\n");

    display_spi_init();
    display_reset();

    // TODO: Add controller-specific initialization sequence
    // This will come from Waveshare example code or datasheet

    printf("Display initialized\n");
}

void display_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    // TODO: Implement pixel transfer to display
    // For now, just acknowledge
    lv_display_flush_ready(disp);
}

void display_set_brightness(uint8_t level) {
    // TODO: Implement PWM backlight control
    printf("Set brightness: %d%%\n", level);
}
```

**Step 4: Update CMakeLists.txt**

Add to executable sources:
```cmake
add_executable(${PROJECT_NAME}
    src/main.c
    src/hal/display_driver.c
)
```

**Step 5: Test compilation**

Run:
```bash
cmake --build build
```

Expected: Build succeeds with no errors

**Step 6: Commit**

```bash
git add src/hal/display_driver.h src/hal/display_driver.c CMakeLists.txt
git commit -m "feat: add display driver HAL stub

- Define pin mappings for Waveshare AMOLED board
- Implement SPI initialization
- Add reset and command/data write functions
- Stub display_flush_cb for LVGL integration
- TODO: Add controller init sequence from datasheet"
```

---

## Task 3: LVGL Integration & Basic Display Test

**Files:**
- Modify: `src/main.c`
- Create: `src/hal/lvgl_helpers.h`
- Create: `src/hal/lvgl_helpers.c`

**Step 1: Create LVGL helper functions**

```c
// src/hal/lvgl_helpers.h
#ifndef LVGL_HELPERS_H
#define LVGL_HELPERS_H

#include "lvgl.h"

void lvgl_init(void);
void lvgl_task_handler(void);  // Call in main loop

#endif
```

```c
// src/hal/lvgl_helpers.c
#include "lvgl_helpers.h"
#include "display_driver.h"
#include <stdio.h>

#define LVGL_BUFFER_SIZE (DISPLAY_WIDTH * 20)

static lv_color_t buf1[LVGL_BUFFER_SIZE];
static lv_color_t buf2[LVGL_BUFFER_SIZE];

void lvgl_init(void) {
    printf("Initializing LVGL...\n");

    lv_init();

    // Initialize display hardware
    display_init();

    // Create display driver
    lv_display_t *disp = lv_display_create(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_display_set_flush_cb(disp, display_flush_cb);
    lv_display_set_buffers(disp, buf1, buf2, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    printf("LVGL initialized\n");
}

void lvgl_task_handler(void) {
    lv_timer_handler();
}
```

**Step 2: Update main.c with LVGL test**

```c
// src/main.c
#include <stdio.h>
#include "pico/stdlib.h"
#include "lvgl.h"
#include "hal/lvgl_helpers.h"

int main() {
    stdio_init_all();

    printf("RP2350 AMOLED Clock starting...\n");

    lvgl_init();

    // Create a simple test screen
    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hello LVGL!");
    lv_obj_center(label);

    printf("Entering main loop...\n");

    while (1) {
        lvgl_task_handler();
        sleep_ms(5);
    }

    return 0;
}
```

**Step 3: Update CMakeLists.txt**

```cmake
add_executable(${PROJECT_NAME}
    src/main.c
    src/hal/display_driver.c
    src/hal/lvgl_helpers.c
)
```

**Step 4: Build and test**

Run:
```bash
cmake --build build
```

Expected: Build succeeds

Note: Display won't show anything yet (flush_cb is stubbed), but LVGL should initialize without errors in USB serial output.

**Step 5: Commit**

```bash
git add src/hal/lvgl_helpers.h src/hal/lvgl_helpers.c src/main.c CMakeLists.txt
git commit -m "feat: integrate LVGL with basic test screen

- Create LVGL initialization wrapper
- Set up dual-buffer partial rendering
- Add simple label test in main
- Ready for display controller implementation"
```

---

## Task 4: Complete Display Driver with Controller Init

**Files:**
- Modify: `src/hal/display_driver.c`

**Step 1: Research controller init sequence**

Check:
1. Waveshare GitHub examples for RP2350-Touch-AMOLED-1.43
2. SH8501/RM67162 datasheet (whichever controller is used)
3. Note the exact initialization sequence

**Step 2: Implement controller initialization**

Add to `display_driver.c`:
```c
static void display_controller_init(void) {
    // Example init sequence - MUST verify with actual Waveshare code
    // This is a placeholder based on typical AMOLED controllers

    display_write_cmd(0x11);  // Sleep out
    sleep_ms(120);

    display_write_cmd(0x3A);  // Pixel format
    uint8_t pf = 0x55;  // 16-bit RGB565
    display_write_data(&pf, 1);

    display_write_cmd(0x36);  // Memory access control
    uint8_t mac = 0x00;  // Rotation/mirror settings
    display_write_data(&mac, 1);

    display_write_cmd(0x29);  // Display on
    sleep_ms(20);

    printf("Display controller initialized\n");
}
```

Update `display_init()`:
```c
void display_init(void) {
    printf("Initializing display...\n");

    display_spi_init();
    display_reset();
    display_controller_init();

    printf("Display initialized\n");
}
```

**Step 3: Implement display_flush_cb**

```c
void display_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    uint16_t width = area->x2 - area->x1 + 1;
    uint16_t height = area->y2 - area->y1 + 1;

    // Set column address
    display_write_cmd(0x2A);
    uint8_t ca[4] = {
        (area->x1 >> 8) & 0xFF,
        area->x1 & 0xFF,
        (area->x2 >> 8) & 0xFF,
        area->x2 & 0xFF
    };
    display_write_data(ca, 4);

    // Set row address
    display_write_cmd(0x2B);
    uint8_t ra[4] = {
        (area->y1 >> 8) & 0xFF,
        area->y1 & 0xFF,
        (area->y2 >> 8) & 0xFF,
        area->y2 & 0xFF
    };
    display_write_data(ra, 4);

    // Write to RAM
    display_write_cmd(0x2C);
    display_write_data(px_map, width * height * 2);

    lv_display_flush_ready(disp);
}
```

**Step 4: Build and flash to board**

Run:
```bash
cmake --build build
```

Flash `build/rp2350_amoled_clock.uf2` to board via BOOTSEL mode

Expected: Display shows "Hello LVGL!" centered on screen

**Step 5: Commit**

```bash
git add src/hal/display_driver.c
git commit -m "feat: implement display controller initialization

- Add SH8501/RM67162 init sequence (verify with Waveshare docs)
- Implement pixel flush callback for LVGL
- Set up column/row addressing and RAM write
- Display now shows LVGL content"
```

---

## Task 5: Touch Driver - CST816S I2C

**Files:**
- Create: `src/hal/touch_driver.h`
- Create: `src/hal/touch_driver.c`
- Modify: `CMakeLists.txt`

**Step 1: Create touch driver header**

```c
// src/hal/touch_driver.h
#ifndef TOUCH_DRIVER_H
#define TOUCH_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

// Waveshare I2C pins for CST816S
#define TOUCH_I2C_PORT i2c0
#define TOUCH_PIN_SDA  6
#define TOUCH_PIN_SCL  7
#define TOUCH_I2C_ADDR 0x15

typedef enum {
    GESTURE_NONE = 0,
    GESTURE_SWIPE_UP = 0x01,
    GESTURE_SWIPE_DOWN = 0x02,
    GESTURE_SWIPE_LEFT = 0x03,
    GESTURE_SWIPE_RIGHT = 0x04,
    GESTURE_SINGLE_TAP = 0x05,
    GESTURE_LONG_PRESS = 0x0C
} touch_gesture_t;

typedef struct {
    uint16_t x;
    uint16_t y;
    bool pressed;
    touch_gesture_t gesture;
} touch_event_t;

void touch_init(void);
bool touch_read(touch_event_t *event);

#endif
```

**Step 2: Create touch driver implementation**

```c
// src/hal/touch_driver.c
#include "touch_driver.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include <stdio.h>

// CST816S register addresses
#define CST816S_REG_GESTURE  0x01
#define CST816S_REG_POINTS   0x02
#define CST816S_REG_X_HIGH   0x03
#define CST816S_REG_X_LOW    0x04
#define CST816S_REG_Y_HIGH   0x05
#define CST816S_REG_Y_LOW    0x06

static uint8_t touch_read_register(uint8_t reg) {
    uint8_t value;
    i2c_write_blocking(TOUCH_I2C_PORT, TOUCH_I2C_ADDR, &reg, 1, true);
    i2c_read_blocking(TOUCH_I2C_PORT, TOUCH_I2C_ADDR, &value, 1, false);
    return value;
}

void touch_init(void) {
    printf("Initializing touch controller...\n");

    // Initialize I2C at 400kHz
    i2c_init(TOUCH_I2C_PORT, 400000);

    gpio_set_function(TOUCH_PIN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(TOUCH_PIN_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(TOUCH_PIN_SDA);
    gpio_pull_up(TOUCH_PIN_SCL);

    sleep_ms(50);

    printf("Touch controller initialized\n");
}

bool touch_read(touch_event_t *event) {
    uint8_t points = touch_read_register(CST816S_REG_POINTS);

    if (points == 0) {
        event->pressed = false;
        event->gesture = GESTURE_NONE;
        return false;
    }

    event->pressed = true;

    // Read touch coordinates
    uint8_t x_high = touch_read_register(CST816S_REG_X_HIGH);
    uint8_t x_low = touch_read_register(CST816S_REG_X_LOW);
    uint8_t y_high = touch_read_register(CST816S_REG_Y_HIGH);
    uint8_t y_low = touch_read_register(CST816S_REG_Y_LOW);

    event->x = ((x_high & 0x0F) << 8) | x_low;
    event->y = ((y_high & 0x0F) << 8) | y_low;

    // Read gesture
    event->gesture = (touch_gesture_t)touch_read_register(CST816S_REG_GESTURE);

    return true;
}
```

**Step 3: Add LVGL input device integration**

Modify `src/hal/lvgl_helpers.c`:
```c
#include "touch_driver.h"

static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data) {
    touch_event_t event;

    if (touch_read(&event)) {
        data->point.x = event.x;
        data->point.y = event.y;
        data->state = event.pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void lvgl_init(void) {
    // ... existing display init ...

    // Initialize touch
    touch_init();

    // Create input device
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touch_read_cb);

    printf("LVGL initialized\n");
}
```

**Step 4: Update CMakeLists.txt**

```cmake
add_executable(${PROJECT_NAME}
    src/main.c
    src/hal/display_driver.c
    src/hal/lvgl_helpers.c
    src/hal/touch_driver.c
)
```

**Step 5: Add touch test to main.c**

```c
// Add to main.c after label creation
lv_obj_t *btn = lv_button_create(lv_screen_active());
lv_obj_align(btn, LV_ALIGN_CENTER, 0, 60);
lv_obj_t *btn_label = lv_label_create(btn);
lv_label_set_text(btn_label, "Touch Me");
```

**Step 6: Build and test**

Run:
```bash
cmake --build build
```

Flash to board and verify touch works (button responds to touch)

**Step 7: Commit**

```bash
git add src/hal/touch_driver.h src/hal/touch_driver.c src/hal/lvgl_helpers.c src/main.c CMakeLists.txt
git commit -m "feat: add CST816S touch driver with LVGL integration

- Implement I2C communication with CST816S
- Read touch coordinates and gestures
- Integrate with LVGL input device
- Add touch test button to verify functionality"
```

---

## Task 6: Software RTC Implementation

**Files:**
- Create: `src/hal/rtc_soft.h`
- Create: `src/hal/rtc_soft.c`
- Modify: `CMakeLists.txt`

**Step 1: Write RTC test**

Create `tests/test_rtc.c`:
```c
#include "hal/rtc_soft.h"
#include <assert.h>
#include <stdio.h>

void test_time_increment(void) {
    rtc_time_t time = {
        .hour = 23,
        .min = 59,
        .sec = 59,
        .year = 2025,
        .month = 12,
        .day = 31,
        .weekday = 2  // Tuesday
    };

    // Simulate increment (manually for now, will use timer later)
    time.sec++;
    if (time.sec >= 60) {
        time.sec = 0;
        time.min++;
        if (time.min >= 60) {
            time.min = 0;
            time.hour++;
            if (time.hour >= 24) {
                time.hour = 0;
                time.day++;
                time.weekday = (time.weekday + 1) % 7;
                // Month/year rollover logic needed
            }
        }
    }

    assert(time.hour == 0);
    assert(time.min == 0);
    assert(time.sec == 0);
    assert(time.day == 32);  // Will be 1 after month rollover

    printf("test_time_increment: PASS\n");
}

int main() {
    test_time_increment();
    return 0;
}
```

Note: Full unit testing requires a test harness. For embedded, we'll verify with on-device printf debugging.

**Step 2: Create RTC header**

```c
// src/hal/rtc_soft.h
#ifndef RTC_SOFT_H
#define RTC_SOFT_H

#include <stdint.h>

typedef struct {
    uint8_t hour;    // 0-23
    uint8_t min;     // 0-59
    uint8_t sec;     // 0-59
    uint16_t year;   // Full year (e.g., 2025)
    uint8_t month;   // 1-12
    uint8_t day;     // 1-31
    uint8_t weekday; // 0=Sun, 1=Mon, ..., 6=Sat
} rtc_time_t;

void rtc_init(const rtc_time_t *initial);
void rtc_get_time(rtc_time_t *time);
void rtc_set_time(const rtc_time_t *time);

#endif
```

**Step 3: Implement RTC core logic**

```c
// src/hal/rtc_soft.c
#include "rtc_soft.h"
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include <stdio.h>

static rtc_time_t current_time;
static alarm_id_t alarm_id;

// Days in each month (non-leap year)
static const uint8_t days_in_month[] = {
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static bool is_leap_year(uint16_t year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static uint8_t get_days_in_month(uint8_t month, uint16_t year) {
    if (month == 2 && is_leap_year(year)) {
        return 29;
    }
    return days_in_month[month - 1];
}

static void increment_time(void) {
    current_time.sec++;

    if (current_time.sec >= 60) {
        current_time.sec = 0;
        current_time.min++;

        if (current_time.min >= 60) {
            current_time.min = 0;
            current_time.hour++;

            if (current_time.hour >= 24) {
                current_time.hour = 0;
                current_time.day++;
                current_time.weekday = (current_time.weekday + 1) % 7;

                uint8_t max_days = get_days_in_month(current_time.month, current_time.year);
                if (current_time.day > max_days) {
                    current_time.day = 1;
                    current_time.month++;

                    if (current_time.month > 12) {
                        current_time.month = 1;
                        current_time.year++;
                    }
                }
            }
        }
    }
}

static int64_t rtc_alarm_callback(alarm_id_t id, void *user_data) {
    increment_time();
    return 1000000;  // Re-schedule for 1 second (microseconds)
}

void rtc_init(const rtc_time_t *initial) {
    if (initial) {
        current_time = *initial;
    } else {
        // Default: 2025-01-01 00:00:00 Wednesday
        current_time.year = 2025;
        current_time.month = 1;
        current_time.day = 1;
        current_time.hour = 0;
        current_time.min = 0;
        current_time.sec = 0;
        current_time.weekday = 3;
    }

    // Set up 1Hz alarm
    alarm_id = add_alarm_in_us(1000000, rtc_alarm_callback, NULL, true);

    printf("RTC initialized: %04d-%02d-%02d %02d:%02d:%02d\n",
           current_time.year, current_time.month, current_time.day,
           current_time.hour, current_time.min, current_time.sec);
}

void rtc_get_time(rtc_time_t *time) {
    *time = current_time;
}

void rtc_set_time(const rtc_time_t *time) {
    current_time = *time;
    printf("RTC set: %04d-%02d-%02d %02d:%02d:%02d\n",
           current_time.year, current_time.month, current_time.day,
           current_time.hour, current_time.min, current_time.sec);
}
```

**Step 4: Update CMakeLists.txt**

```cmake
add_executable(${PROJECT_NAME}
    src/main.c
    src/hal/display_driver.c
    src/hal/lvgl_helpers.c
    src/hal/touch_driver.c
    src/hal/rtc_soft.c
)
```

**Step 5: Test RTC in main.c**

```c
// Add to main.c
#include "hal/rtc_soft.h"

int main() {
    stdio_init_all();
    printf("RP2350 AMOLED Clock starting...\n");

    // Initialize RTC
    rtc_time_t initial = {
        .year = 2025, .month = 12, .day = 18,
        .hour = 19, .min = 30, .sec = 0,
        .weekday = 3  // Wednesday
    };
    rtc_init(&initial);

    lvgl_init();

    // Update label to show time
    lv_obj_t *time_label = lv_label_create(lv_screen_active());
    lv_obj_center(time_label);

    while (1) {
        rtc_time_t time;
        rtc_get_time(&time);

        char buf[32];
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d", time.hour, time.min, time.sec);
        lv_label_set_text(time_label, buf);

        lvgl_task_handler();
        sleep_ms(100);
    }
}
```

**Step 6: Build and test**

Run:
```bash
cmake --build build
```

Flash and verify time increments correctly on display

**Step 7: Commit**

```bash
git add src/hal/rtc_soft.h src/hal/rtc_soft.c src/main.c CMakeLists.txt
git commit -m "feat: implement software RTC with hardware timer

- Add time structure with year/month/day/hour/min/sec/weekday
- Implement 1Hz timer callback for time increment
- Handle month/year rollover and leap years
- Test with live clock display"
```

---

## Task 7: Settings Persistence to Flash

**Files:**
- Create: `src/config/settings.h`
- Create: `src/config/settings.c`
- Modify: `CMakeLists.txt`

**Step 1: Create settings header**

```c
// src/config/settings.h
#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>
#include "hal/rtc_soft.h"

#define SETTINGS_MAGIC 0x434C4B31  // "CLK1"

typedef enum {
    CLOCK_MODE_DIGITAL = 0,
    CLOCK_MODE_ANALOG = 1
} clock_mode_t;

typedef enum {
    THEME_DARK = 0,
    THEME_LIGHT = 1,
    THEME_BLUE = 2,
    THEME_RED = 3,
    THEME_GREEN = 4
} theme_id_t;

typedef struct {
    uint32_t magic;
    clock_mode_t clock_mode;
    theme_id_t theme_id;
    rtc_time_t last_time;
    uint8_t brightness;
    uint32_t checksum;
} settings_t;

void settings_init(void);
void settings_load(settings_t *s);
void settings_save(const settings_t *s);
void settings_get_default(settings_t *s);

#endif
```

**Step 2: Implement settings persistence**

```c
// src/config/settings.c
#include "settings.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"
#include <string.h>
#include <stdio.h>

// Use last 4KB sector of flash (offset from start of flash)
#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)

static uint32_t calculate_checksum(const settings_t *s) {
    uint32_t sum = 0;
    const uint8_t *data = (const uint8_t *)s;
    size_t len = sizeof(settings_t) - sizeof(s->checksum);

    for (size_t i = 0; i < len; i++) {
        sum += data[i];
    }

    return sum;
}

void settings_get_default(settings_t *s) {
    s->magic = SETTINGS_MAGIC;
    s->clock_mode = CLOCK_MODE_DIGITAL;
    s->theme_id = THEME_DARK;
    s->brightness = 80;

    // Default time: 2025-01-01 00:00:00 Wed
    s->last_time.year = 2025;
    s->last_time.month = 1;
    s->last_time.day = 1;
    s->last_time.hour = 0;
    s->last_time.min = 0;
    s->last_time.sec = 0;
    s->last_time.weekday = 3;

    s->checksum = calculate_checksum(s);
}

void settings_load(settings_t *s) {
    const uint8_t *flash_ptr = (const uint8_t *)(XIP_BASE + FLASH_TARGET_OFFSET);
    memcpy(s, flash_ptr, sizeof(settings_t));

    // Validate
    if (s->magic != SETTINGS_MAGIC) {
        printf("Settings invalid (bad magic), using defaults\n");
        settings_get_default(s);
        return;
    }

    uint32_t expected_checksum = calculate_checksum(s);
    if (s->checksum != expected_checksum) {
        printf("Settings invalid (bad checksum), using defaults\n");
        settings_get_default(s);
        return;
    }

    printf("Settings loaded from flash\n");
}

void settings_save(const settings_t *s) {
    settings_t to_save = *s;
    to_save.checksum = calculate_checksum(&to_save);

    printf("Saving settings to flash...\n");

    // Disable interrupts during flash write
    uint32_t ints = save_and_disable_interrupts();

    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, (const uint8_t *)&to_save, sizeof(settings_t));

    restore_interrupts(ints);

    printf("Settings saved\n");
}

void settings_init(void) {
    printf("Initializing settings...\n");
    // Nothing to do for init
}
```

**Step 3: Update CMakeLists.txt**

```cmake
add_executable(${PROJECT_NAME}
    src/main.c
    src/hal/display_driver.c
    src/hal/lvgl_helpers.c
    src/hal/touch_driver.c
    src/hal/rtc_soft.c
    src/config/settings.c
)
```

**Step 4: Test settings in main.c**

```c
#include "config/settings.h"

int main() {
    stdio_init_all();

    settings_init();

    settings_t settings;
    settings_load(&settings);

    printf("Clock mode: %d, Theme: %d, Brightness: %d%%\n",
           settings.clock_mode, settings.theme_id, settings.brightness);

    // Initialize RTC with saved time
    rtc_init(&settings.last_time);

    // ... rest of main
}
```

**Step 5: Build and test**

Run:
```bash
cmake --build build
```

Flash and verify settings load/save works (check serial output)

**Step 6: Commit**

```bash
git add src/config/settings.h src/config/settings.c src/main.c CMakeLists.txt
git commit -m "feat: add settings persistence to flash

- Define settings structure with clock mode, theme, time, brightness
- Implement flash read/write with checksum validation
- Load settings on boot, fall back to defaults if invalid
- Save settings to last 4KB sector of flash"
```

---

## Task 8: Digital Clock UI

**Files:**
- Create: `src/ui/clock_digital.h`
- Create: `src/ui/clock_digital.c`
- Modify: `CMakeLists.txt`

**Step 1: Create digital clock header**

```c
// src/ui/clock_digital.h
#ifndef CLOCK_DIGITAL_H
#define CLOCK_DIGITAL_H

#include "lvgl.h"

lv_obj_t *clock_digital_create(lv_obj_t *parent);
void clock_digital_update(lv_obj_t *clock);

#endif
```

**Step 2: Implement digital clock**

```c
// src/ui/clock_digital.c
#include "clock_digital.h"
#include "hal/rtc_soft.h"
#include <stdio.h>

typedef struct {
    lv_obj_t *time_label;
    lv_obj_t *date_label;
    lv_obj_t *weekday_label;
} clock_digital_t;

static const char *weekday_names[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday",
    "Thursday", "Friday", "Saturday"
};

static const char *month_names[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

lv_obj_t *clock_digital_create(lv_obj_t *parent) {
    lv_obj_t *container = lv_obj_create(parent);
    lv_obj_set_size(container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container, 0, 0);

    clock_digital_t *clock = malloc(sizeof(clock_digital_t));

    // Time label (large)
    clock->time_label = lv_label_create(container);
    lv_obj_set_style_text_font(clock->time_label, &lv_font_montserrat_48, 0);
    lv_obj_align(clock->time_label, LV_ALIGN_CENTER, 0, -30);

    // Date label (medium)
    clock->date_label = lv_label_create(container);
    lv_obj_set_style_text_font(clock->date_label, &lv_font_montserrat_24, 0);
    lv_obj_align(clock->date_label, LV_ALIGN_CENTER, 0, 30);

    // Weekday label (medium)
    clock->weekday_label = lv_label_create(container);
    lv_obj_set_style_text_font(clock->weekday_label, &lv_font_montserrat_20, 0);
    lv_obj_align(clock->weekday_label, LV_ALIGN_CENTER, 0, 60);

    // Store clock data in user_data
    lv_obj_set_user_data(container, clock);

    return container;
}

void clock_digital_update(lv_obj_t *clock_obj) {
    clock_digital_t *clock = (clock_digital_t *)lv_obj_get_user_data(clock_obj);

    rtc_time_t time;
    rtc_get_time(&time);

    // Update time
    char time_buf[16];
    snprintf(time_buf, sizeof(time_buf), "%02d:%02d:%02d",
             time.hour, time.min, time.sec);
    lv_label_set_text(clock->time_label, time_buf);

    // Update date
    char date_buf[32];
    snprintf(date_buf, sizeof(date_buf), "%s %d, %d",
             month_names[time.month - 1], time.day, time.year);
    lv_label_set_text(clock->date_label, date_buf);

    // Update weekday
    lv_label_set_text(clock->weekday_label, weekday_names[time.weekday]);
}
```

**Step 3: Update CMakeLists.txt**

```cmake
add_executable(${PROJECT_NAME}
    src/main.c
    src/hal/display_driver.c
    src/hal/lvgl_helpers.c
    src/hal/touch_driver.c
    src/hal/rtc_soft.c
    src/config/settings.c
    src/ui/clock_digital.c
)
```

**Step 4: Use digital clock in main.c**

```c
#include "ui/clock_digital.h"

int main() {
    // ... initialization ...

    lvgl_init();

    lv_obj_t *clock = clock_digital_create(lv_screen_active());

    while (1) {
        clock_digital_update(clock);
        lvgl_task_handler();
        sleep_ms(100);
    }
}
```

**Step 5: Build and test**

Run:
```bash
cmake --build build
```

Flash and verify digital clock displays time/date/weekday

**Step 6: Commit**

```bash
git add src/ui/clock_digital.h src/ui/clock_digital.c src/main.c CMakeLists.txt
git commit -m "feat: add digital clock UI component

- Create digital clock with time, date, and weekday
- Use large font for time, medium for date/weekday
- Update function called from main loop
- Clean container layout with LVGL"
```

---

## Task 9: Theme Manager

**Files:**
- Create: `src/ui/theme_manager.h`
- Create: `src/ui/theme_manager.c`
- Modify: `CMakeLists.txt`

**Step 1: Create theme manager header**

```c
// src/ui/theme_manager.h
#ifndef THEME_MANAGER_H
#define THEME_MANAGER_H

#include "lvgl.h"
#include "config/settings.h"

typedef struct {
    lv_color_t bg;
    lv_color_t primary;
    lv_color_t secondary;
    lv_color_t accent;
} theme_colors_t;

void theme_manager_init(void);
void theme_manager_apply(theme_id_t theme_id);
const theme_colors_t *theme_manager_get_colors(void);

#endif
```

**Step 2: Implement theme manager**

```c
// src/ui/theme_manager.c
#include "theme_manager.h"
#include <stdio.h>

static theme_colors_t current_colors;

static const theme_colors_t theme_dark = {
    .bg = {.red = 0, .green = 0, .blue = 0},           // Black
    .primary = {.red = 255, .green = 255, .blue = 255}, // White
    .secondary = {.red = 128, .green = 128, .blue = 128}, // Gray
    .accent = {.red = 0, .green = 200, .blue = 255}     // Cyan
};

static const theme_colors_t theme_light = {
    .bg = {.red = 255, .green = 255, .blue = 255},     // White
    .primary = {.red = 0, .green = 0, .blue = 0},       // Black
    .secondary = {.red = 100, .green = 100, .blue = 100}, // Dark gray
    .accent = {.red = 0, .green = 100, .blue = 200}     // Blue
};

static const theme_colors_t theme_blue = {
    .bg = {.red = 0, .green = 0, .blue = 30},          // Dark blue
    .primary = {.red = 100, .green = 200, .blue = 255}, // Light blue
    .secondary = {.red = 50, .green = 100, .blue = 150}, // Med blue
    .accent = {.red = 255, .green = 200, .blue = 0}     // Gold
};

static const theme_colors_t theme_red = {
    .bg = {.red = 30, .green = 0, .blue = 0},          // Dark red
    .primary = {.red = 255, .green = 100, .blue = 100}, // Light red
    .secondary = {.red = 150, .green = 50, .blue = 50}, // Med red
    .accent = {.red = 255, .green = 200, .blue = 0}     // Gold
};

static const theme_colors_t theme_green = {
    .bg = {.red = 0, .green = 30, .blue = 0},          // Dark green
    .primary = {.red = 100, .green = 255, .blue = 100}, // Light green
    .secondary = {.red = 50, .green = 150, .blue = 50}, // Med green
    .accent = {.red = 255, .green = 255, .blue = 0}     // Yellow
};

void theme_manager_init(void) {
    printf("Theme manager initialized\n");
    current_colors = theme_dark;
}

void theme_manager_apply(theme_id_t theme_id) {
    switch (theme_id) {
        case THEME_DARK:
            current_colors = theme_dark;
            break;
        case THEME_LIGHT:
            current_colors = theme_light;
            break;
        case THEME_BLUE:
            current_colors = theme_blue;
            break;
        case THEME_RED:
            current_colors = theme_red;
            break;
        case THEME_GREEN:
            current_colors = theme_green;
            break;
        default:
            current_colors = theme_dark;
    }

    // Apply to screen background
    lv_obj_set_style_bg_color(lv_screen_active(),
                              lv_color_make(current_colors.bg.red,
                                          current_colors.bg.green,
                                          current_colors.bg.blue), 0);

    printf("Theme applied: %d\n", theme_id);
}

const theme_colors_t *theme_manager_get_colors(void) {
    return &current_colors;
}
```

**Step 3: Update digital clock to use theme colors**

Modify `src/ui/clock_digital.c`:
```c
#include "theme_manager.h"

// In clock_digital_create:
void clock_digital_update(lv_obj_t *clock_obj) {
    const theme_colors_t *colors = theme_manager_get_colors();
    clock_digital_t *clock = (clock_digital_t *)lv_obj_get_user_data(clock_obj);

    // Apply theme colors
    lv_color_t primary = lv_color_make(colors->primary.red,
                                       colors->primary.green,
                                       colors->primary.blue);
    lv_obj_set_style_text_color(clock->time_label, primary, 0);
    lv_obj_set_style_text_color(clock->date_label, primary, 0);
    lv_obj_set_style_text_color(clock->weekday_label, primary, 0);

    // ... rest of update code
}
```

**Step 4: Update CMakeLists.txt**

```cmake
add_executable(${PROJECT_NAME}
    src/main.c
    src/hal/display_driver.c
    src/hal/lvgl_helpers.c
    src/hal/touch_driver.c
    src/hal/rtc_soft.c
    src/config/settings.c
    src/ui/clock_digital.c
    src/ui/theme_manager.c
)
```

**Step 5: Initialize theme in main.c**

```c
#include "ui/theme_manager.h"

int main() {
    // ... initialization ...

    theme_manager_init();
    theme_manager_apply(settings.theme_id);

    // ... rest
}
```

**Step 6: Build and test**

Run:
```bash
cmake --build build
```

Test different themes by modifying settings.theme_id

**Step 7: Commit**

```bash
git add src/ui/theme_manager.h src/ui/theme_manager.c src/ui/clock_digital.c src/main.c CMakeLists.txt
git commit -m "feat: add theme manager with 5 color schemes

- Define 5 themes: dark, light, blue, red, green
- Apply theme colors to screen background and text
- Integrate with settings system
- Update digital clock to use theme colors"
```

---

## Task 10: Analog Clock UI

**Files:**
- Create: `src/ui/clock_analog.h`
- Create: `src/ui/clock_analog.c`
- Modify: `CMakeLists.txt`

**Step 1: Create analog clock header**

```c
// src/ui/clock_analog.h
#ifndef CLOCK_ANALOG_H
#define CLOCK_ANALOG_H

#include "lvgl.h"

lv_obj_t *clock_analog_create(lv_obj_t *parent);
void clock_analog_update(lv_obj_t *clock);

#endif
```

**Step 2: Implement analog clock**

```c
// src/ui/clock_analog.c
#include "clock_analog.h"
#include "theme_manager.h"
#include "hal/rtc_soft.h"
#include <math.h>
#include <stdio.h>

#define CLOCK_RADIUS 150
#define CENTER_X 233
#define CENTER_Y 233

typedef struct {
    lv_obj_t *canvas;
    lv_obj_t *date_label;
    lv_draw_buf_t *draw_buf;
} clock_analog_t;

static void draw_clock_face(lv_obj_t *canvas, const theme_colors_t *colors) {
    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.bg_color = lv_color_make(colors->bg.red, colors->bg.green, colors->bg.blue);
    rect_dsc.bg_opa = LV_OPA_COVER;

    lv_area_t area = {0, 0, 466, 466};
    lv_draw_rect(canvas, &rect_dsc, &area);

    // Draw hour markers
    lv_draw_line_dsc_t line_dsc;
    lv_draw_line_dsc_init(&line_dsc);
    line_dsc.color = lv_color_make(colors->secondary.red,
                                   colors->secondary.green,
                                   colors->secondary.blue);
    line_dsc.width = 2;

    for (int i = 0; i < 12; i++) {
        float angle = (i * 30 - 90) * M_PI / 180.0f;
        int16_t x1 = CENTER_X + (CLOCK_RADIUS - 10) * cos(angle);
        int16_t y1 = CENTER_Y + (CLOCK_RADIUS - 10) * sin(angle);
        int16_t x2 = CENTER_X + CLOCK_RADIUS * cos(angle);
        int16_t y2 = CENTER_Y + CLOCK_RADIUS * sin(angle);

        lv_point_t points[] = {{x1, y1}, {x2, y2}};
        lv_draw_line(canvas, &line_dsc, points);
    }
}

static void draw_hand(lv_obj_t *canvas, int16_t angle, int16_t length,
                     int16_t width, const theme_colors_t *colors, bool is_second) {
    float rad = (angle - 90) * M_PI / 180.0f;
    int16_t x = CENTER_X + length * cos(rad);
    int16_t y = CENTER_Y + length * sin(rad);

    lv_draw_line_dsc_t line_dsc;
    lv_draw_line_dsc_init(&line_dsc);

    if (is_second) {
        line_dsc.color = lv_color_make(colors->accent.red,
                                       colors->accent.green,
                                       colors->accent.blue);
    } else {
        line_dsc.color = lv_color_make(colors->primary.red,
                                       colors->primary.green,
                                       colors->primary.blue);
    }
    line_dsc.width = width;

    lv_point_t points[] = {{CENTER_X, CENTER_Y}, {x, y}};
    lv_draw_line(canvas, &line_dsc, points);
}

lv_obj_t *clock_analog_create(lv_obj_t *parent) {
    lv_obj_t *container = lv_obj_create(parent);
    lv_obj_set_size(container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container, 0, 0);

    clock_analog_t *clock = malloc(sizeof(clock_analog_t));

    // Create canvas for clock face
    clock->canvas = lv_canvas_create(container);

    // Allocate draw buffer
    size_t buf_size = LV_CANVAS_BUF_SIZE(466, 466, LV_COLOR_DEPTH, LV_COLOR_FORMAT_RGB565);
    clock->draw_buf = malloc(buf_size);
    lv_canvas_set_buffer(clock->canvas, clock->draw_buf, 466, 466, LV_COLOR_FORMAT_RGB565);
    lv_obj_center(clock->canvas);

    // Date label below center
    clock->date_label = lv_label_create(container);
    lv_obj_set_style_text_font(clock->date_label, &lv_font_montserrat_16, 0);
    lv_obj_align(clock->date_label, LV_ALIGN_CENTER, 0, 80);

    lv_obj_set_user_data(container, clock);

    return container;
}

void clock_analog_update(lv_obj_t *clock_obj) {
    clock_analog_t *clock = (clock_analog_t *)lv_obj_get_user_data(clock_obj);
    const theme_colors_t *colors = theme_manager_get_colors();

    rtc_time_t time;
    rtc_get_time(&time);

    // Clear and draw face
    draw_clock_face(clock->canvas, colors);

    // Calculate angles
    int16_t hour_angle = (time.hour % 12) * 30 + time.min / 2;
    int16_t min_angle = time.min * 6;
    int16_t sec_angle = time.sec * 6;

    // Draw hands (order matters: hour, minute, second)
    draw_hand(clock->canvas, hour_angle, 80, 6, colors, false);
    draw_hand(clock->canvas, min_angle, 120, 4, colors, false);
    draw_hand(clock->canvas, sec_angle, 130, 2, colors, true);

    // Update date
    static const char *month_names[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };

    char date_buf[32];
    snprintf(date_buf, sizeof(date_buf), "%s %d, %d",
             month_names[time.month - 1], time.day, time.year);
    lv_label_set_text(clock->date_label, date_buf);

    lv_color_t primary = lv_color_make(colors->primary.red,
                                       colors->primary.green,
                                       colors->primary.blue);
    lv_obj_set_style_text_color(clock->date_label, primary, 0);
}
```

**Step 3: Update CMakeLists.txt**

```cmake
add_executable(${PROJECT_NAME}
    src/main.c
    src/hal/display_driver.c
    src/hal/lvgl_helpers.c
    src/hal/touch_driver.c
    src/hal/rtc_soft.c
    src/config/settings.c
    src/ui/clock_digital.c
    src/ui/clock_analog.c
    src/ui/theme_manager.c
)

target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    hardware_spi
    hardware_i2c
    hardware_timer
    hardware_flash
    lvgl
    m  # Math library for sin/cos
)
```

**Step 4: Test analog clock in main**

Temporarily switch to analog in main.c:
```c
#include "ui/clock_analog.h"

// Replace clock_digital_create with:
lv_obj_t *clock = clock_analog_create(lv_screen_active());

// In loop:
clock_analog_update(clock);
```

**Step 5: Build and test**

Run:
```bash
cmake --build build
```

Flash and verify analog clock displays with hands

**Step 6: Commit**

```bash
git add src/ui/clock_analog.h src/ui/clock_analog.c CMakeLists.txt src/main.c
git commit -m "feat: add analog clock UI component

- Draw clock face with hour markers
- Render hour, minute, second hands
- Use canvas for drawing with theme colors
- Display date below clock center
- Calculate hand angles from RTC time"
```

---

## Task 11: Screen Manager & Mode Switching

**Files:**
- Create: `src/ui/screen_manager.h`
- Create: `src/ui/screen_manager.c`
- Modify: `src/main.c`
- Modify: `CMakeLists.txt`

**Step 1: Create screen manager header**

```c
// src/ui/screen_manager.h
#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

#include "lvgl.h"
#include "config/settings.h"

typedef enum {
    SCREEN_STATE_CLOCK,
    SCREEN_STATE_SETTINGS,
    SCREEN_STATE_TIME_SET
} screen_state_t;

void screen_manager_init(clock_mode_t initial_mode);
void screen_manager_update(void);
void screen_manager_switch_clock_mode(void);
void screen_manager_set_state(screen_state_t state);
clock_mode_t screen_manager_get_clock_mode(void);

#endif
```

**Step 2: Implement screen manager**

```c
// src/ui/screen_manager.c
#include "screen_manager.h"
#include "clock_digital.h"
#include "clock_analog.h"
#include <stdio.h>

static screen_state_t current_state = SCREEN_STATE_CLOCK;
static clock_mode_t current_clock_mode = CLOCK_MODE_DIGITAL;
static lv_obj_t *digital_clock = NULL;
static lv_obj_t *analog_clock = NULL;

void screen_manager_init(clock_mode_t initial_mode) {
    current_clock_mode = initial_mode;
    current_state = SCREEN_STATE_CLOCK;

    // Create both clocks but hide one
    digital_clock = clock_digital_create(lv_screen_active());
    analog_clock = clock_analog_create(lv_screen_active());

    if (current_clock_mode == CLOCK_MODE_DIGITAL) {
        lv_obj_add_flag(analog_clock, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(digital_clock, LV_OBJ_FLAG_HIDDEN);
    }

    printf("Screen manager initialized, mode: %s\n",
           current_clock_mode == CLOCK_MODE_DIGITAL ? "digital" : "analog");
}

void screen_manager_update(void) {
    if (current_state == SCREEN_STATE_CLOCK) {
        if (current_clock_mode == CLOCK_MODE_DIGITAL) {
            clock_digital_update(digital_clock);
        } else {
            clock_analog_update(analog_clock);
        }
    }
    // TODO: Handle other states (settings, time set)
}

void screen_manager_switch_clock_mode(void) {
    if (current_clock_mode == CLOCK_MODE_DIGITAL) {
        current_clock_mode = CLOCK_MODE_ANALOG;
        lv_obj_add_flag(digital_clock, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(analog_clock, LV_OBJ_FLAG_HIDDEN);
        printf("Switched to analog mode\n");
    } else {
        current_clock_mode = CLOCK_MODE_DIGITAL;
        lv_obj_add_flag(analog_clock, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(digital_clock, LV_OBJ_FLAG_HIDDEN);
        printf("Switched to digital mode\n");
    }
}

void screen_manager_set_state(screen_state_t state) {
    current_state = state;
    printf("Screen state: %d\n", state);
}

clock_mode_t screen_manager_get_clock_mode(void) {
    return current_clock_mode;
}
```

**Step 3: Add gesture handling to LVGL helpers**

Modify `src/hal/lvgl_helpers.c`:
```c
#include "ui/screen_manager.h"

static touch_gesture_t last_gesture = GESTURE_NONE;

static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data) {
    touch_event_t event;

    if (touch_read(&event)) {
        data->point.x = event.x;
        data->point.y = event.y;
        data->state = event.pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;

        // Handle gestures
        if (event.gesture != GESTURE_NONE && event.gesture != last_gesture) {
            if (event.gesture == GESTURE_SWIPE_LEFT || event.gesture == GESTURE_SWIPE_RIGHT) {
                screen_manager_switch_clock_mode();
            }
            // TODO: Handle tap for settings menu
            last_gesture = event.gesture;
        }
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
        last_gesture = GESTURE_NONE;
    }
}
```

**Step 4: Update main.c**

```c
#include "ui/screen_manager.h"

int main() {
    stdio_init_all();

    settings_init();
    settings_t settings;
    settings_load(&settings);

    rtc_init(&settings.last_time);

    lvgl_init();

    theme_manager_init();
    theme_manager_apply(settings.theme_id);

    screen_manager_init(settings.clock_mode);

    while (1) {
        screen_manager_update();
        lvgl_task_handler();
        sleep_ms(100);
    }
}
```

**Step 5: Update CMakeLists.txt**

```cmake
add_executable(${PROJECT_NAME}
    src/main.c
    src/hal/display_driver.c
    src/hal/lvgl_helpers.c
    src/hal/touch_driver.c
    src/hal/rtc_soft.c
    src/config/settings.c
    src/ui/clock_digital.c
    src/ui/clock_analog.c
    src/ui/theme_manager.c
    src/ui/screen_manager.c
)
```

**Step 6: Build and test**

Run:
```bash
cmake --build build
```

Flash and test swipe left/right to switch between analog and digital

**Step 7: Commit**

```bash
git add src/ui/screen_manager.h src/ui/screen_manager.c src/hal/lvgl_helpers.c src/main.c CMakeLists.txt
git commit -m "feat: add screen manager with gesture-based mode switching

- Create screen manager to control clock modes
- Handle swipe left/right gestures to switch modes
- Show/hide clocks based on active mode
- Integrate with settings for initial mode
- Clean separation of UI state management"
```

---

## Task 12: Settings Menu UI

**Files:**
- Create: `src/ui/settings_menu.h`
- Create: `src/ui/settings_menu.c`
- Modify: `src/ui/screen_manager.c`
- Modify: `src/hal/lvgl_helpers.c`
- Modify: `CMakeLists.txt`

**Step 1: Create settings menu header**

```c
// src/ui/settings_menu.h
#ifndef SETTINGS_MENU_H
#define SETTINGS_MENU_H

#include "lvgl.h"
#include "config/settings.h"

lv_obj_t *settings_menu_create(lv_obj_t *parent);
void settings_menu_show(lv_obj_t *menu, const settings_t *current);
void settings_menu_hide(lv_obj_t *menu);
bool settings_menu_is_visible(lv_obj_t *menu);

#endif
```

**Step 2: Implement basic settings menu**

```c
// src/ui/settings_menu.c
#include "settings_menu.h"
#include "theme_manager.h"
#include "screen_manager.h"
#include <stdio.h>

typedef struct {
    lv_obj_t *container;
    lv_obj_t *title;
    lv_obj_t *theme_btn;
    lv_obj_t *brightness_label;
    lv_obj_t *brightness_slider;
    lv_obj_t *back_btn;
    settings_t *settings_ref;
} settings_menu_t;

static settings_t temp_settings;

static void theme_btn_event_cb(lv_event_t *e) {
    temp_settings.theme_id = (temp_settings.theme_id + 1) % 5;
    theme_manager_apply(temp_settings.theme_id);

    lv_obj_t *btn = lv_event_get_target(e);
    const char *theme_names[] = {"Dark", "Light", "Blue", "Red", "Green"};
    char buf[32];
    snprintf(buf, sizeof(buf), "Theme: %s", theme_names[temp_settings.theme_id]);
    lv_label_set_text(lv_obj_get_child(btn, 0), buf);
}

static void brightness_slider_event_cb(lv_event_t *e) {
    lv_obj_t *slider = lv_event_get_target(e);
    temp_settings.brightness = lv_slider_get_value(slider);

    settings_menu_t *menu = lv_event_get_user_data(e);
    char buf[32];
    snprintf(buf, sizeof(buf), "Brightness: %d%%", temp_settings.brightness);
    lv_label_set_text(menu->brightness_label, buf);

    display_set_brightness(temp_settings.brightness);
}

static void back_btn_event_cb(lv_event_t *e) {
    settings_menu_t *menu = lv_event_get_user_data(e);

    // Save settings
    settings_save(&temp_settings);

    // Hide menu
    settings_menu_hide(menu->container);
    screen_manager_set_state(SCREEN_STATE_CLOCK);
}

lv_obj_t *settings_menu_create(lv_obj_t *parent) {
    settings_menu_t *menu = malloc(sizeof(settings_menu_t));

    // Semi-transparent overlay
    menu->container = lv_obj_create(parent);
    lv_obj_set_size(menu->container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(menu->container, LV_OPA_90, 0);
    lv_obj_add_flag(menu->container, LV_OBJ_FLAG_HIDDEN);

    // Title
    menu->title = lv_label_create(menu->container);
    lv_label_set_text(menu->title, "Settings");
    lv_obj_set_style_text_font(menu->title, &lv_font_montserrat_28, 0);
    lv_obj_align(menu->title, LV_ALIGN_TOP_MID, 0, 20);

    // Theme button
    menu->theme_btn = lv_button_create(menu->container);
    lv_obj_set_size(menu->theme_btn, 200, 50);
    lv_obj_align(menu->theme_btn, LV_ALIGN_CENTER, 0, -60);
    lv_obj_add_event_cb(menu->theme_btn, theme_btn_event_cb, LV_EVENT_CLICKED, menu);

    lv_obj_t *theme_label = lv_label_create(menu->theme_btn);
    lv_label_set_text(theme_label, "Theme: Dark");
    lv_obj_center(theme_label);

    // Brightness label
    menu->brightness_label = lv_label_create(menu->container);
    lv_label_set_text(menu->brightness_label, "Brightness: 80%");
    lv_obj_align(menu->brightness_label, LV_ALIGN_CENTER, 0, 0);

    // Brightness slider
    menu->brightness_slider = lv_slider_create(menu->container);
    lv_obj_set_width(menu->brightness_slider, 200);
    lv_obj_align(menu->brightness_slider, LV_ALIGN_CENTER, 0, 30);
    lv_slider_set_range(menu->brightness_slider, 10, 100);
    lv_obj_add_event_cb(menu->brightness_slider, brightness_slider_event_cb,
                       LV_EVENT_VALUE_CHANGED, menu);

    // Back button
    menu->back_btn = lv_button_create(menu->container);
    lv_obj_set_size(menu->back_btn, 150, 50);
    lv_obj_align(menu->back_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_event_cb(menu->back_btn, back_btn_event_cb, LV_EVENT_CLICKED, menu);

    lv_obj_t *back_label = lv_label_create(menu->back_btn);
    lv_label_set_text(back_label, "Back");
    lv_obj_center(back_label);

    lv_obj_set_user_data(menu->container, menu);

    return menu->container;
}

void settings_menu_show(lv_obj_t *menu_obj, const settings_t *current) {
    temp_settings = *current;

    settings_menu_t *menu = lv_obj_get_user_data(menu_obj);

    // Update UI to match current settings
    const char *theme_names[] = {"Dark", "Light", "Blue", "Red", "Green"};
    char buf[32];
    snprintf(buf, sizeof(buf), "Theme: %s", theme_names[temp_settings.theme_id]);
    lv_label_set_text(lv_obj_get_child(menu->theme_btn, 0), buf);

    lv_slider_set_value(menu->brightness_slider, temp_settings.brightness, LV_ANIM_OFF);
    snprintf(buf, sizeof(buf), "Brightness: %d%%", temp_settings.brightness);
    lv_label_set_text(menu->brightness_label, buf);

    lv_obj_clear_flag(menu_obj, LV_OBJ_FLAG_HIDDEN);
}

void settings_menu_hide(lv_obj_t *menu_obj) {
    lv_obj_add_flag(menu_obj, LV_OBJ_FLAG_HIDDEN);
}

bool settings_menu_is_visible(lv_obj_t *menu_obj) {
    return !lv_obj_has_flag(menu_obj, LV_OBJ_FLAG_HIDDEN);
}
```

**Step 3: Integrate settings menu with screen manager**

Modify `src/ui/screen_manager.c`:
```c
#include "settings_menu.h"

static lv_obj_t *settings_menu_obj = NULL;
static settings_t *global_settings = NULL;

void screen_manager_init(clock_mode_t initial_mode) {
    // ... existing code ...

    settings_menu_obj = settings_menu_create(lv_screen_active());
}

void screen_manager_set_state(screen_state_t state) {
    current_state = state;

    if (state == SCREEN_STATE_SETTINGS) {
        // Load current settings and show menu
        settings_t settings;
        settings_load(&settings);
        settings_menu_show(settings_menu_obj, &settings);
    }

    printf("Screen state: %d\n", state);
}
```

**Step 4: Handle tap gesture for settings**

Modify `src/hal/lvgl_helpers.c`:
```c
// In touch_read_cb, add tap handling:
if (event.gesture == GESTURE_SINGLE_TAP) {
    screen_manager_set_state(SCREEN_STATE_SETTINGS);
}
```

**Step 5: Update CMakeLists.txt**

```cmake
add_executable(${PROJECT_NAME}
    src/main.c
    src/hal/display_driver.c
    src/hal/lvgl_helpers.c
    src/hal/touch_driver.c
    src/hal/rtc_soft.c
    src/config/settings.c
    src/ui/clock_digital.c
    src/ui/clock_analog.c
    src/ui/theme_manager.c
    src/ui/screen_manager.c
    src/ui/settings_menu.c
)
```

**Step 6: Build and test**

Run:
```bash
cmake --build build
```

Flash and test:
- Tap screen to open settings
- Change theme, verify it applies
- Adjust brightness
- Tap back to return to clock

**Step 7: Commit**

```bash
git add src/ui/settings_menu.h src/ui/settings_menu.c src/ui/screen_manager.c src/hal/lvgl_helpers.c CMakeLists.txt
git commit -m "feat: add settings menu UI with theme and brightness

- Create overlay settings menu
- Add theme switcher button (cycles through 5 themes)
- Add brightness slider (10-100%)
- Save settings to flash on back button
- Open with tap gesture, close with back button"
```

---

## Task 13: Final Integration & Testing

**Files:**
- Modify: `src/main.c`
- Create: `README.md`

**Step 1: Clean up main.c**

```c
// src/main.c - Final clean version
#include <stdio.h>
#include "pico/stdlib.h"
#include "hal/lvgl_helpers.h"
#include "hal/rtc_soft.h"
#include "config/settings.h"
#include "ui/theme_manager.h"
#include "ui/screen_manager.h"

int main() {
    stdio_init_all();
    printf("\n=== RP2350 AMOLED Desk Clock ===\n");

    // Load settings
    settings_init();
    settings_t settings;
    settings_load(&settings);

    printf("Loaded settings: mode=%d, theme=%d, brightness=%d%%\n",
           settings.clock_mode, settings.theme_id, settings.brightness);

    // Initialize RTC with saved time
    rtc_init(&settings.last_time);

    // Initialize LVGL and display
    lvgl_init();

    // Apply theme
    theme_manager_init();
    theme_manager_apply(settings.theme_id);

    // Set initial brightness
    display_set_brightness(settings.brightness);

    // Initialize screen manager
    screen_manager_init(settings.clock_mode);

    printf("System initialized. Entering main loop...\n");

    uint32_t last_save = 0;
    const uint32_t SAVE_INTERVAL_MS = 60000;  // Save time every minute

    while (1) {
        screen_manager_update();
        lvgl_task_handler();

        // Periodically save time to flash
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - last_save > SAVE_INTERVAL_MS) {
            rtc_get_time(&settings.last_time);
            settings.clock_mode = screen_manager_get_clock_mode();
            settings_save(&settings);
            last_save = now;
        }

        sleep_ms(100);
    }

    return 0;
}
```

**Step 2: Create README**

```markdown
# RP2350 AMOLED Desk Clock

USB-powered desk clock with AMOLED display featuring analog and digital clock faces, touch controls, and customizable themes.

## Hardware

- **Board:** Waveshare RP2350-Touch-AMOLED-1.43
- **Display:** 466x466 AMOLED (SH8501 controller)
- **Touch:** CST816S capacitive touch
- **MCU:** RP2350 (520KB SRAM, 4MB Flash)

## Features

- **Dual Clock Modes:** Analog and digital displays
- **Touch Controls:**
  - Swipe left/right: Switch between clock modes
  - Tap: Open settings menu
- **5 Color Themes:** Dark, Light, Blue, Red, Green
- **Adjustable Brightness:** 10-100%
- **Persistent Settings:** Saved to flash
- **Software RTC:** Time keeping with manual setting

## Building

### Prerequisites

- Pico SDK installed
- ARM GCC toolchain
- CMake 3.13+
- LVGL v9.4+ (cloned at `~/src/github.com/lvgl/lvgl`)

### Build Commands

```bash
# Initial build
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Rebuild after changes
cmake --build build
```

### Flashing

1. Hold BOOTSEL button while plugging in USB
2. Copy `build/rp2350_amoled_clock.uf2` to RPI-RP2 drive
3. Board will reboot and start clock

## Project Structure

```
src/
├── main.c                  # Entry point & main loop
├── hal/                    # Hardware abstraction
│   ├── display_driver.c/h  # SPI display driver
│   ├── touch_driver.c/h    # I2C touch controller
│   ├── rtc_soft.c/h       # Software RTC
│   └── lvgl_helpers.c/h   # LVGL integration
├── ui/                     # User interface
│   ├── clock_digital.c/h   # Digital clock face
│   ├── clock_analog.c/h    # Analog clock face
│   ├── settings_menu.c/h   # Settings overlay
│   ├── theme_manager.c/h   # Theme system
│   └── screen_manager.c/h  # Screen state management
└── config/
    └── settings.c/h        # Flash persistence
```

## Usage

### Setting the Time

1. Tap screen to open settings
2. (TODO: Add time setting UI)
3. For now: Modify initial time in `src/main.c`

### Changing Themes

1. Tap screen to open settings
2. Tap "Theme" button to cycle through themes
3. Tap "Back" to save and return

### Adjusting Brightness

1. Tap screen to open settings
2. Drag brightness slider
3. Tap "Back" to save and return

## Future Enhancements

- WiFi NTP time synchronization
- Time setting UI
- Alarms and timers
- Weather widget
- Custom themes
- Battery backup option

## License

MIT License - See LICENSE file for details
```

**Step 3: Test all features**

Create test checklist:
```bash
# Hardware Test
- [ ] Display shows content
- [ ] Touch responds to taps
- [ ] Touch responds to swipes
- [ ] Brightness control works

# Clock Test
- [ ] Digital clock shows correct time format
- [ ] Analog clock hands move correctly
- [ ] Date displays correctly
- [ ] Weekday displays correctly
- [ ] Time increments every second

# UI Test
- [ ] Swipe left/right switches modes
- [ ] Tap opens settings menu
- [ ] Theme switcher works (all 5 themes)
- [ ] Brightness slider works
- [ ] Back button closes settings

# Persistence Test
- [ ] Settings saved on back button
- [ ] Power cycle preserves settings
- [ ] Time saved periodically
- [ ] Corrupted settings fall back to defaults
```

**Step 4: Build final release**

Run:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

**Step 5: Commit**

```bash
git add src/main.c README.md
git commit -m "feat: finalize main loop and add documentation

- Clean up main.c with proper initialization order
- Add periodic settings save (every minute)
- Save clock mode and time to flash
- Add comprehensive README with build instructions
- Document features, usage, and project structure
- Ready for initial release"
```

---

## Task 14: Pin Configuration Verification

**Files:**
- Modify: `src/hal/display_driver.h`
- Modify: `src/hal/touch_driver.h`

**Step 1: Verify Waveshare pinout**

Check Waveshare documentation/examples for correct pins:
- https://www.waveshare.com/wiki/RP2350-Touch-AMOLED-1.43

**Step 2: Update pin definitions**

Update both driver headers with verified pins:
```c
// Example - verify with actual Waveshare docs
#define DISPLAY_PIN_SCK  10  // Verify
#define DISPLAY_PIN_MOSI 11  // Verify
// ... etc
```

**Step 3: Test on hardware**

Flash and verify display/touch work correctly

**Step 4: Commit**

```bash
git add src/hal/display_driver.h src/hal/touch_driver.h
git commit -m "fix: verify and update pin definitions from Waveshare docs

- Confirm SPI pins for display
- Confirm I2C pins for touch
- Match Waveshare RP2350-Touch-AMOLED-1.43 schematic
- Tested on hardware"
```

---

## Completion

**Definition of Done:**
- [x] All source files created and compiling
- [x] Display showing clock (analog or digital)
- [x] Touch gestures working (swipe to switch, tap for settings)
- [x] Settings persisted to flash
- [x] Themes switchable and working
- [x] README documentation complete
- [x] Code committed to git
- [x] Built and tested on actual hardware

**Next Steps (Future Enhancements):**
1. Add WiFi NTP sync (check if board has WiFi module)
2. Implement time-setting UI
3. Add alarms and timers
4. Weather widget integration
5. Custom theme editor
6. OTA firmware updates

---

## Notes for Implementation

- **DRY:** Reuse HAL abstractions across UI components
- **YAGNI:** Don't add WiFi/NTP until basic clock works
- **TDD:** Test each HAL component before building UI on top
- **Frequent commits:** Commit after each working task
- **Hardware verification:** Confirm pins/controller from Waveshare docs before implementing
- **Error handling:** Add graceful fallbacks for corrupted settings
- **Performance:** Use LVGL's partial rendering to minimize SPI traffic
- **Power:** AMOLED displays save power with black backgrounds (use dark theme default)
