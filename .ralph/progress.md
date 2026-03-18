
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
