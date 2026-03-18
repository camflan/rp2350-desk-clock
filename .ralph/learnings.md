
## Build environment

- Pico SDK 2.1.1 does not include a board definition for `waveshare_rp2350_touch_amoled_143`; using generic `pico2` board with `rp2350-arm-s` platform
- ARM toolchain is at `/usr/local/bin/arm-none-eabi-gcc` (v9.2.1)
- SDK is at `~/src/github.com/raspberrypi/pico-sdk` — `PICO_SDK_PATH` must be set for cmake
- picotool is built from source during cmake configure if not installed system-wide

## Mise configuration

- `github:raspberrypi/pico-sdk-tools` releases contain multiple assets (openocd, picotool, riscv-toolchain, etc.); must use `asset_pattern` to select the right one
- The ARM toolchain at `/usr/local/bin/arm-none-eabi-gcc` is from the Playdate SDK (v9.2.1), not from `gcc-arm-embedded` cask
- `gcc-arm-embedded` cask installs to `/Applications/ArmGNUToolchain/<version>/arm-none-eabi/` and requires sudo (pkg installer)
- Mise `env` section: use `{{env.HOME}}` template syntax instead of `~` for paths that need expansion by non-shell tools like cmake

## LVGL v9

- `LV_HOR_RES_MAX`/`LV_VER_RES_MAX` were removed in v9; display resolution is set at runtime via `lv_display_create(w, h)`
- `lv_conf.h` placement: set `LV_CONF_PATH` in CMake to the absolute path; LVGL's `custom.cmake` adds the parent dir to includes automatically
- Disable `LV_CONF_BUILD_DISABLE_EXAMPLES`, `LV_CONF_BUILD_DISABLE_DEMOS`, and `LV_CONF_BUILD_DISABLE_THORVG_INTERNAL` to cut build time significantly
- LVGL v9.2.2 submodule pinned at commit `7f07a129e`

## PIO QSPI

- PIO byte-writes (`*(volatile uint8_t *)&pio->txf[sm]`) replicate the byte 4× across the 32-bit FIFO word — critical for understanding nibble ordering
- With left-shift (`shift_right=false`), the high nibble of the replicated byte shifts out first, low nibble second
- For 1-bit mode: place the earlier bit at position 4 (high nibble) and the later bit at position 0 (low nibble) of each FIFO byte
- TX FIFO empty does not mean transmission complete — must also check TX-stall flag via `fdebug` register
- `pico_generate_pio_header()` must appear after `add_executable()` in CMake
- The `pio_qspi.pio.h` header is generated into the build directory; include via `"pio_qspi.pio.h"` (CMake adds the build dir to includes automatically)

## CO5300 display driver

- CO5300 QSPI protocol embeds command/data distinction in the data stream (no DC pin needed): command packet `[0x02, 0x00, cmd, 0x00, ...data]`, bulk pixel write `[0x32, 0x00, 0x2C, 0x00, ...pixels]`
- Command packets sent in 1-bit mode; pixel data in 4-bit mode for throughput
- Display requires 6px X offset when setting column address window (0x2A command)
- Brightness via 0x51 command: maps 0–100% to 0x25–0xFF; default init value 0xA0
- RGB565 pixels must be byte-swapped to big-endian before sending
- Pin assignments for Waveshare RP2350-Touch-AMOLED-1.43: CS=15, DC=8 (unused), RST=16, BL=19, SCK=10, D0=11, D1=12, D2=13, D3=14
