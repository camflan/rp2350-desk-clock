
## Task 1: Project scaffold — DONE

- Created CMakeLists.txt targeting RP2350 via `pico2` board (waveshare_rp2350_touch_amoled_143 not in SDK 2.1.1 board defs)
- Empty `src/main.c` with `stdio_init_all()` and tight loop
- `pico_sdk_import.cmake` copied from SDK
- Directory structure: `src/hal/`, `src/ui/`, `src/config/`, `lib/`, `docs/`
- USB stdio enabled, UART stdio disabled
- `CMAKE_EXPORT_COMPILE_COMMANDS ON` for clangd
- Build produces `desk_clock.uf2` (43KB) and `compile_commands.json`
- Cloned pico-sdk 2.1.1 to `~/src/github.com/raspberrypi/pico-sdk` (with submodules)
- Committed: 9e165ee

## Task 2: Mise toolchain setup — DONE

- `.mise.toml` installs cmake (latest) and picotool 2.1.1 via `github:raspberrypi/pico-sdk-tools` with `asset_pattern` to select the picotool zip
- `PICO_SDK_PATH` and `PICO_TOOLCHAIN_PATH` env vars set (SDK path uses `{{env.HOME}}` template for expansion)
- `mise run setup` clones pico-sdk with `--recursive` if missing, installs `gcc-arm-embedded` cask via Homebrew with clear error if brew not found
- `mise run build` runs cmake configure + build; `mise run clean` removes build dir
- `bin_path = "picotool"` configured so mise finds the binary inside the extracted archive
- `version_prefix = "v"` handles the `v` prefix on pico-sdk-tools release tags
- Committed: 95fcdf1

## Task 3: Flash and serial workflow — DONE

- Added `mise run flash` task: loads ELF via `picotool load -f` then `picotool reboot`
- Added `mise run serial` task: auto-detects `/dev/tty.usbmodem*`, opens with `screen` at 115200 baud, clear error if no device found
- Added `mise run dev` task: composes build → flash → serial via `depends`
- USB stdio was already enabled and UART disabled in CMakeLists.txt (from Task 1)
- Created README.md with BOOTSEL first-flash instructions and task reference

## Task 4: LVGL integration — DONE

- Added LVGL v9.2.2 as git submodule at `lib/lvgl`
- Created `lib/lv_conf.h` with `LV_COLOR_DEPTH 16`, `LV_DPI_DEF 320`, `LV_MEM_SIZE (128 * 1024)`
- `LV_HOR_RES_MAX`/`LV_VER_RES_MAX` don't exist in LVGL v9 — resolution set at runtime via `lv_display_create(466, 466)` in Task 10
- CMake `lvgl` target linked to `desk_clock` executable
- Disabled examples, demos, and thorvg builds to reduce compile time
- Disabled unused color formats (kept RGB565, ARGB8888, A8)
- Enabled widgets needed for clock UI (label, canvas, arc, roller, slider, switch, button)
- Project compiles cleanly, produces `desk_clock.uf2`
- Committed: 171d6eb

## Task 5: PIO QSPI driver — DONE

- Created `src/hal/pio_qspi.pio` — PIO assembly program that shifts 4 bits per clock on D0–D3 with CLK as side-set
- Created `src/hal/pio_qspi.h` and `src/hal/pio_qspi.c` — init, 1-bit write, and 4-bit write functions
- `pio_qspi_init(clk_pin, d0_pin, baudrate)` calculates clock divider from system clock for target baudrate (75MHz)
- 1-bit mode: expands each byte into 4 FIFO pushes (8 clocks), placing each data bit on D0 only (MSB first)
- 4-bit mode: pushes bytes directly — each yields 2 nibble shifts on D0–D3
- `drain_tx()` waits for FIFO empty + TX-stall flag to ensure all data is shifted out
- No dynamic allocation — all static state
- `pico_generate_pio_header` wired in CMakeLists.txt; `hardware_pio` linked
- Removed `src/hal/.gitkeep` placeholder

## Task 6: Display driver — DONE

- Created `src/config/display.h` with `DISPLAY_BUF_PIXELS (466 * 10)` constant
- Created `src/hal/display_driver.h` with pin definitions (CS=15, DC=8, RST=16, BL=19, SCK=10, D0=11)
- Created `src/hal/display_driver.c` with full CO5300 init sequence: sleep-out, 0xC4/0x44/0x35/0x53 config, display-on, brightness 0xA0, inversion-off, MADCTL, pixel format RGB565
- `display_flush_cb` uses static buffer (`4 + DISPLAY_BUF_PIXELS * 2`), byte-swaps RGB565 to big-endian, sends header in 1-bit mode + pixels in 4-bit mode
- 6px X offset applied via `LCD_X_OFFSET` in `set_window()`
- `display_set_brightness(0–100)` maps to CO5300 0x51 command (0x25–0xFF range)
- `display_test_fill(color)` fills in 256-pixel chunks to avoid stack overflow
- Smoke test in `main.c`: red → green → blue on boot
- Added `display_driver.c` to CMake, linked `hardware_gpio`
- Build produces desk_clock.uf2 (48KB)

## Task 7: Touch driver — DONE

- Created `src/hal/touch_driver.h` and `src/hal/touch_driver.c`
- IC is FT6146 (CST816S-register-compatible) at I2C address 0x15
- Pin assignments verified from Waveshare schematic V2.0: SDA=6, SCL=7, INT=21, RST=22
- Uses `i2c1` at 400kHz, reset pulse on init, INT pin polled
- Gesture mapping: raw 0x03→SWIPE_LEFT, 0x04→SWIPE_RIGHT, 0x05/0x0B/0x0C→TAP
- `hardware_i2c` linked in CMake
- Committed: 275b5e2

## Task 8: Software RTC — DONE

- RP2350 has NO hardware RTC — implemented software RTC using `get_absolute_time()`
- Created `src/hal/rtc_driver.h` and `src/hal/rtc_driver.c` with `rtc_app_` prefix
- Stores base datetime + base timestamp, calculates current time via elapsed seconds
- Full date rollover including leap years (Tomohiko Sakamoto day-of-week algorithm)
- Default boot time: 2026-03-18 12:00:00 Wednesday
- Committed: 5142035

## Task 10: LVGL display and tick wiring — DONE

- Replaced smoke test with LVGL init in `src/main.c`
- `lv_tick_set_cb` with `to_ms_since_boot(get_absolute_time())`
- Dual static render buffers (DISPLAY_BUF_PIXELS × 2 bytes each), partial render mode
- "Hello Clock!" test label confirmed on hardware
- Committed: 742da6b

## Task 12: Analog clock face — DONE

- Created `src/ui/clock_analog.h` and `src/ui/clock_analog.c`
- 12 hour markers via `lv_line`, thicker at 12/3/6/9
- Hour (100px, 6px), minute (150px, 4px), second (170px, 2px red) hands via `lv_line`
- Date label centered 60px below center
- 1s LVGL timer drives updates; tracks `last_second` to skip no-ops
- Full-screen `lv_obj_invalidate(screen)` as workaround for LVGL line AA edge pixels leaking outside bounding boxes during partial rendering (see Task 18)
- SDL simulator added (`sim/`, `mise run sim`) for desktop LVGL development
- Committed: 4c0488f, 4c30cc9

## Other fixes

- Flash task: dropped `-u` (nounset) flag, switched to `.uf2`, ignore picotool reboot exit code
- Committed: 6231464, 9bd1a32
