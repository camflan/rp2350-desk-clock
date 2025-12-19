# RP2350 AMOLED Desk Clock - Design Document

**Date:** 2025-12-18
**Board:** Waveshare RP2350-Touch-AMOLED-1.43
**Status:** Initial Design

## Overview

A USB-powered desk clock with AMOLED display, featuring both analog and digital clock faces, multiple themes, and touch interaction. Built using Pico SDK and LVGL v9.4+.

## Requirements

### Core Features (Initial Version)
- Always-on clock display (USB-powered)
- Two clock modes: digital and analog
- Display information: time, date, day of week
- Touch interaction:
  - Swipe left/right: switch between analog/digital modes
  - Tap: open settings menu
- Multiple themes (switchable in settings)
- Manual time setting via touch interface
- Settings persistence to flash

### Future Expansion
- WiFi NTP time sync (if board supports WiFi module)
- Additional widgets (weather, timers, etc.)
- More interactive features

## Technology Stack

- **Hardware:** Waveshare RP2350-Touch-AMOLED-1.43
  - Display: 466x466 AMOLED (likely SH8501 controller)
  - Touch: CST816S capacitive touch controller
  - MCU: RP2350 (520KB SRAM, 4MB Flash)
- **SDK:** Raspberry Pi Pico SDK (C/C++)
- **UI Framework:** LVGL v9.4+
- **Build System:** CMake
- **Development:** Neovim with clangd LSP

## Architecture

### Project Structure

```
rp2350-amoled/
├── CMakeLists.txt          # Main build config
├── pico_sdk_import.cmake   # Pico SDK integration
├── src/
│   ├── main.c              # Entry point & main loop
│   ├── hal/                # Hardware abstraction
│   │   ├── display_driver.c/h
│   │   ├── touch_driver.c/h
│   │   └── rtc_soft.c/h    # Software RTC using timers
│   ├── ui/                 # LVGL UI components
│   │   ├── clock_analog.c/h
│   │   ├── clock_digital.c/h
│   │   ├── settings_menu.c/h
│   │   └── theme_manager.c/h
│   └── config/
│       └── settings.c/h    # Persistence to flash
├── lib/
│   └── lvgl/              # LVGL as git submodule or symlink
└── docs/
    └── plans/
```

### Build System

- **CMake Integration:**
  - Pico SDK's CMake toolchain
  - LVGL as library target
  - Generate `compile_commands.json` for clangd
  - Support for SWD debugging

- **Dependencies:**
  - Pico SDK (hardware APIs, SPI, I2C, timers)
  - LVGL v9.4+ (existing clone at ~/src/github.com/lvgl)
  - Display controller driver (SH8501 or similar)

## Hardware Abstraction Layer

### Display Driver

**Controller:** SH8501 or similar AMOLED controller (to be confirmed from Waveshare docs)

**Interface:**
```c
void display_init(void);
void display_flush(lv_area_t *area, lv_color_t *color_p);
void display_set_brightness(uint8_t level);  // 0-100%
void display_sleep(bool enable);
```

**Implementation:**
- SPI communication at high speed (62.5 MHz+)
- DMA transfers for framebuffer to free CPU
- RGB565 color format (16-bit)
- No RP2350 framebuffer needed (controller has built-in)

**Display Specs:**
- Resolution: 466x466 (circular display area)
- SPI interface
- High DPI (~320 DPI)

### Touch Driver

**Controller:** CST816S capacitive touch controller

**Interface:**
```c
typedef struct {
    uint16_t x, y;
    bool pressed;
    uint8_t gesture;  // 0=none, 1=swipe_left, 2=swipe_right, 3=tap
} touch_event_t;

void touch_init(void);
bool touch_read(touch_event_t *event);
```

**Implementation:**
- I2C communication (slave address: 0x15)
- Poll at 50-100Hz in main loop
- Hardware gesture detection support
- Map gestures to UI actions:
  - Swipe left/right: switch clock modes
  - Single tap: open settings
  - Long press: future expansion

### Software RTC

**Interface:**
```c
typedef struct {
    uint8_t hour, min, sec;
    uint16_t year;
    uint8_t month, day, weekday;
} rtc_time_t;

void rtc_init(rtc_time_t *initial);
void rtc_get_time(rtc_time_t *time);
void rtc_set_time(rtc_time_t *time);
```

**Implementation:**
- Hardware timer interrupt at 1Hz
- Software time increment with rollover logic
- Persist time offset to flash when manually set
- On boot: restore from flash + estimate elapsed time
- Expected accuracy: ±50ppm (~4 seconds/day drift)

## LVGL Integration

### Configuration

Key `lv_conf.h` settings:
```c
#define LV_COLOR_DEPTH 16          // RGB565
#define LV_HOR_RES_MAX 466
#define LV_VER_RES_MAX 466
#define LV_DPI_DEF 320
#define LV_MEM_SIZE (128 * 1024)   // 128KB LVGL heap
#define LV_USE_DRAW_SW 1
```

### Display Driver Registration

```c
void lvgl_init(void) {
    lv_init();
    lv_display_t *disp = lv_display_create(466, 466);
    lv_display_set_flush_cb(disp, display_flush);
    lv_display_set_buffers(disp, buf1, buf2, BUFFER_SIZE,
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
}
```

### Timing

- Hardware timer for 1ms tick
- Call `lv_timer_handler()` every ~5ms in main loop
- Simple superloop architecture (no RTOS)

### Memory Strategy

- Partial rendering with dual buffers (10-20KB each)
- Separate LVGL heap from system malloc
- No full framebuffer needed

## UI Architecture

### Screen States

```c
typedef enum {
    SCREEN_CLOCK,      // Main clock display
    SCREEN_SETTINGS,   // Settings menu
    SCREEN_TIME_SET    // Time adjustment
} screen_state_t;
```

### Clock Faces

**Digital Clock:**
- Large time display (HH:MM:SS) with custom font
- Medium date display (Mon, Dec 18, 2025)
- Prominent day of week
- Update every second via timer callback

**Analog Clock:**
- Clock hands using LVGL canvas or line objects
- 12 hour markers
- Smooth second hand motion
- Date display below center
- Redraw only when changed (optimization)

### Theme System

```c
typedef struct {
    lv_color_t bg;         // Background (black for AMOLED)
    lv_color_t primary;    // Main text/hands
    lv_color_t secondary;  // Date/accents
    lv_color_t accent;     // Highlights
} theme_colors_t;
```

**Predefined Themes:**
- Dark (AMOLED-optimized)
- Light
- Blue
- Red
- Green
- Custom (future)

### Navigation

- Swipe gestures: switch between analog/digital (stay in SCREEN_CLOCK)
- Tap: transition to SCREEN_SETTINGS
- Back button in settings: return to SCREEN_CLOCK

## Settings & Persistence

### Settings Structure

```c
typedef struct {
    uint32_t magic;              // Validation marker
    uint8_t clock_mode;          // 0=digital, 1=analog
    uint8_t theme_id;            // Theme selection
    struct {
        uint8_t hour, min, sec;
        uint16_t year;
        uint8_t month, day, weekday;
    } time_offset;               // Last set time
    uint8_t brightness;          // 0-100
    uint32_t checksum;           // Data integrity
} settings_t;
```

### Flash Storage

- Use last sector of flash (~4KB at `PICO_FLASH_SIZE_BYTES - 4096`)
- Pico SDK flash API: `flash_range_erase()`, `flash_range_program()`
- Load on boot, save on change (with debounce)
- Checksum validation (fall back to defaults if corrupted)

## Testing Strategy

1. **Hardware Bring-up:**
   - Verify SPI communication to display
   - Verify I2C communication to touch controller
   - Test basic GPIO/LED

2. **Display Testing:**
   - Fill screen with solid colors
   - Verify no dead pixels
   - Test brightness levels

3. **Touch Calibration:**
   - Draw touch points on screen
   - Verify coordinate accuracy
   - Test gesture detection

4. **Clock Accuracy:**
   - Long-running test (24+ hours)
   - Measure time drift

5. **Settings Persistence:**
   - Power cycle testing
   - Verify settings retained
   - Test corrupted data recovery

## Debugging

- **SWD:** Via picoprobe for breakpoints and debugging
- **UART:** Printf debugging over USB serial
- **LVGL Profiling:** Built-in performance monitoring

## Future Enhancements

- WiFi NTP time synchronization (check for WiFi module on board)
- Additional widgets (weather, calendar, timers)
- Ambient light sensor for auto-brightness
- Battery backup for RTC (optional hardware mod)
- OTA firmware updates (if WiFi available)
- Custom watch faces / themes
- Alarms and notifications

## Development Workflow

1. **Setup:** CMake + Pico SDK + LVGL integration
2. **Build:** `cmake -B build && cmake --build build`
3. **Flash:** Copy UF2 to BOOTSEL mode or use picoprobe
4. **Edit:** Neovim with clangd LSP support
5. **Debug:** SWD via picoprobe or USB serial logging

## Design Decisions

### Why Foundation-First Approach?

Building proper hardware abstraction upfront enables:
- Easy expansion to WiFi/NTP later
- Clean separation of hardware and UI layers
- Simpler testing and debugging
- Better code organization for future features

### Why LVGL?

- Mature, well-documented UI framework
- Excellent performance on embedded systems
- Rich widget library and theming support
- Active community and examples

### Why Software RTC?

- Simple initial implementation
- No additional hardware required
- Sufficient accuracy for desk clock
- Easy to migrate to NTP sync later
- Persistence provides reasonable boot recovery

### Why USB-Powered?

- Simpler implementation (no battery management)
- Always-on display practical at desk
- Eliminates sleep mode complexity
- Can add battery later if needed
