# Tasks

## Task [1]: Project scaffold

- **Goal:** Establish the base CMake project structure that compiles cleanly for RP2350
- **Acceptance criteria:** `cmake -B build && cmake --build build` produces a `.uf2` with an empty `main.c`; `compile_commands.json` is generated; directory structure matches design doc (`src/hal/`, `src/ui/`, `src/config/`, `lib/`, `docs/`); written from scratch — `feature/desk-clock-implementation` is reference only, no code copied
- **Blocked by:** none
- **Passing:** yes

## Task [2]: Mise toolchain setup

- **Goal:** Reproducible dev environment via mise with documented fallbacks for tools not available in mise
- **Acceptance criteria:** `.mise.toml` installs cmake and picotool (via `github:raspberrypi/pico-sdk-tools` GitHub release asset); `mise run setup` script clones pico-sdk with `--recursive` to `~/src/github.com/raspberrypi/pico-sdk` if not present, installs ARM toolchain via `brew install --cask gcc-arm-embedded` with clear messaging if Homebrew is missing; `PICO_SDK_PATH` and `PICO_TOOLCHAIN_PATH` env vars set in `.mise.toml`; `mise run build` and `mise run clean` tasks defined
- **Blocked by:** none
- **Passing:** yes

## Task [3]: Flash and serial workflow

- **Goal:** Enable flash and serial monitoring without unplugging the USB cable
- **Acceptance criteria:** `mise run flash` runs `picotool load -f <elf/uf2> && picotool reboot`; `mise run serial` opens `/dev/tty.usbmodem*` at 115200 baud (exits with clear error if device not found); `mise run dev` composes build + flash + serial; `pico_enable_stdio_usb` enabled and `pico_enable_stdio_uart` disabled in CMakeLists.txt; first-flash BOOTSEL instructions documented in README or task comment
- **Blocked by:** 1, 2
- **Passing:** yes

## Task [4]: LVGL integration

- **Goal:** Add LVGL as a git submodule and configure it for the display
- **Acceptance criteria:** LVGL added as submodule at `lib/lvgl`; `lv_conf.h` configured with `LV_COLOR_DEPTH 16`, `LV_HOR_RES_MAX 466`, `LV_VER_RES_MAX 466`, `LV_DPI_DEF 320`, `LV_MEM_SIZE (128 * 1024)`; CMake `lvgl` target linked to main executable; project compiles with LVGL included
- **Blocked by:** 1
- **Passing:** yes

## Task [5]: PIO QSPI driver

- **Goal:** Implement the PIO-based QSPI driver for CO5300 display communication
- **Acceptance criteria:** `.pio` program implements both 1-bit and 4-bit QSPI write modes; `pio_qspi_init(clk_pin, d0_pin, baudrate)` initializes at 75MHz; `pio_qspi_1bit_write_blocking` and `pio_qspi_4bit_write_blocking` work correctly; no dynamic allocation; `pico_generate_pio_header` wired in CMakeLists.txt; `hardware_pio` linked
- **Blocked by:** 1
- **Passing:** yes

## Task [6]: Display driver

- **Goal:** CO5300 display init and LVGL flush callback with static buffers
- **Acceptance criteria:** `DISPLAY_BUF_PIXELS` constant defined in `src/config/display.h` (initial value: `466 * 10`); `display_init()` executes full CO5300 init sequence over QSPI (sleep-out, pixel format RGB565, display-on, inversion-off); `display_flush_cb` uses a static buffer sized to `4 + DISPLAY_BUF_PIXELS * 2` (no malloc); 6px X offset applied; `display_set_brightness(0–100)` maps to CO5300 0x51 command; `display_test_fill(color)` fills screen with solid color chunked to avoid stack overflow; smoke test in `main.c` fills screen red then green then blue on boot
- **Blocked by:** 4, 5
- **Passing:** yes

## Task [7]: Touch driver

- **Goal:** CST816S capacitive touch controller driver with gesture support
- **Acceptance criteria:** I2C address and pin assignments verified against Waveshare RP2350-Touch-AMOLED-1.43 schematic or official example code before any hardcoding; `touch_init()` initializes CST816S over I2C; `touch_read(touch_event_t *event)` returns x/y coordinates, pressed state, and gesture; gestures mapped: swipe-left=1, swipe-right=2, tap=3; `hardware_i2c` linked in CMake; verified address and pins documented in a comment at top of `touch_driver.c`
- **Blocked by:** 1
- **Passing:** yes

## Task [8]: Hardware RTC

- **Goal:** Wrap RP2350 hardware RTC in a clean application-level API
- **Acceptance criteria:** `rtc_init()` initializes RP2350 hardware RTC via `hardware/rtc.h`; `rtc_get_datetime(rtc_datetime_t *)` and `rtc_set_datetime(const rtc_datetime_t *)` work correctly; `rtc_get_time_string` and `rtc_get_date_string` format to `HH:MM:SS` and `YYYY-MM-DD`; `rtc_get_weekday_string` returns day name; `hardware_rtc` added to CMake link libs; boots with a hardcoded default time
- **Blocked by:** 1
- **Passing:** yes

## Task [9]: Settings persistence

- **Goal:** Persist user settings to flash across power cycles
- **Acceptance criteria:** `settings_t` struct contains magic marker, clock_mode, theme_id, brightness, last-set datetime, and CRC32 checksum; settings stored in last flash sector (`PICO_FLASH_SIZE_BYTES - 4096`); `settings_load()` validates magic and checksum, falls back to defaults on failure; `settings_save()` erases sector then programs; `hardware_flash` linked; power-cycle test: save settings, reboot, verify same values loaded
- **Blocked by:** 8
- **Passing:** no

## Task [10]: LVGL display and tick wiring

- **Goal:** Connect LVGL to the display driver and establish the main loop cadence
- **Acceptance criteria:** `lv_display_create(466, 466)` registered with `display_flush_cb`; dual static render buffers each `DISPLAY_BUF_PIXELS` pixels (from `src/config/display.h`); `lv_tick_set_cb` or repeating timer provides 1ms ticks; `lv_timer_handler()` called every 5ms in main loop; a simple LVGL label renders and displays on screen
- **Blocked by:** 6
- **Passing:** yes

## Task [11]: Digital clock face

- **Goal:** LVGL screen showing digital time, date, and day of week
- **Acceptance criteria:** Screen shows HH:MM:SS in large font, full date (e.g. `Mon, Mar 18 2026`), and day of week; updates exactly once per second via LVGL timer callback; reads time from `rtc_get_datetime`; renders correctly within the 466×466 circular display area (no clipping at corners)
- **Blocked by:** 10
- **Passing:** no

## Task [12]: Analog clock face

- **Goal:** LVGL canvas-based analog clock with hour/minute/second hands and markers
- **Acceptance criteria:** 12 hour markers drawn at correct positions; hour, minute, and second hands drawn at correct angles for current time; second hand updates every second; only redraws when time changes (no unconditional full redraws); date displayed below center; renders within circular 466×466 area
- **Note:** Currently uses full-screen `lv_obj_invalidate(screen)` to work around LVGL line AA edge pixel leaking. See Task 18 for optimization.
- **Blocked by:** 10
- **Passing:** yes

## Task [13]: Theme system

- **Goal:** Centralized color theme management applied across all screens
- **Acceptance criteria:** `theme_colors_t` struct with bg, primary, secondary, accent fields; 5 predefined themes: dark (AMOLED black), light, blue, red, green; `theme_apply(uint8_t theme_id)` updates all active LVGL objects; current theme retrievable via `theme_get_current_id()`
- **Blocked by:** 10
- **Passing:** no

## Task [14]: Settings menu UI

- **Goal:** Touch-accessible settings screen with theme, brightness, and clock mode controls
- **Acceptance criteria:** Settings screen contains: theme picker (cycles through 5 themes with preview), brightness slider (0–100, calls `display_set_brightness`), clock mode toggle (digital/analog), back button returning to clock screen; all changes persist immediately via `settings_save()`
- **Blocked by:** 11, 12, 13
- **Passing:** no

## Task [15]: Time set UI

- **Goal:** Touch UI for manually setting the current time and date
- **Acceptance criteria:** Screen accessible from settings menu; hour, minute, second roller widgets with wrap-around; year, month, day rollers; confirm button calls `rtc_set_datetime()` and `settings_save()` then returns to settings; cancel discards changes; rollers initialized to current RTC time
- **Blocked by:** 14, 8
- **Passing:** no

## Task [16]: Navigation and gesture routing

- **Goal:** Wire touch gestures to screen transitions and clock mode switching
- **Acceptance criteria:** Swipe left/right on clock screen switches between analog and digital modes (no screen transition, in-place swap); single tap on clock screen navigates to settings screen; back button in settings returns to clock screen; all transitions use LVGL screen load with appropriate animation; gesture source is `touch_read()` polled in main loop
- **Blocked by:** 14, 7
- **Passing:** no

## Task [17]: Main loop integration

- **Goal:** Wire all modules into a complete, working desk clock
- **Acceptance criteria:** Boot sequence: settings load → rtc init (restore saved time if valid) → display init → touch init → LVGL init → show last-used clock face with last-used theme; superloop calls `lv_timer_handler()` every 5ms and `touch_read()` at 50–100Hz; clock displays correct time and updates live; gestures navigate correctly; brightness and theme from settings applied on boot
- **Blocked by:** 16, 9, 15
- **Passing:** no

## Task [18]: Optimize hand invalidation for higher refresh rates

- **Goal:** Replace full-screen `lv_obj_invalidate(screen)` with per-hand bounding rect invalidation so sweep rates above 1Hz are feasible
- **Acceptance criteria:** Second hand update at 5Hz without visible artifacts or frame drops; only the old and new hand bounding rects (plus AA padding) are invalidated per update; no stale pixels from anti-aliased line edges
- **Blocked by:** 12
- **Passing:** no

## Task [19]: Configurable second hand sweep mode

- **Goal:** Let user choose second hand beat rate: 1Hz (quartz tick), 3Hz (21,600 vph), 4Hz (28,800 vph), 5Hz (36,000 vph), smooth (Spring Drive style)
- **Acceptance criteria:** `sweep_mode_t` enum with modes; update timer period adjusts to match selected frequency; smooth mode uses highest feasible rate; setting persisted via settings system; selectable from settings menu
- **Blocked by:** 18, 14
- **Passing:** no
