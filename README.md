# RP2350 AMOLED Desk Clock

USB-powered desk clock with AMOLED display featuring analog and digital clock faces, touch controls, and customizable themes.

## Hardware

- **Board:** Waveshare RP2350-Touch-AMOLED-1.43
- **Display:** 466x466 AMOLED (CO5300 controller)
- **Touch:** FT6146 capacitive touch
- **MCU:** RP2350 (520KB SRAM, 4MB Flash)

## Quick Start

This project uses [mise](https://mise.jdx.dev/) for dependency and task management.

### First Time Setup

1. **Install mise** (if not already installed):
   ```bash
   curl https://mise.jdx.dev/install.sh | sh
   ```

2. **Clone and enter the project**:
   ```bash
   cd /path/to/rp2350-amoled
   ```

3. **Install dependencies**:
   ```bash
   mise run setup
   ```

4. **Configure the build**:
   ```bash
   mise run configure
   ```

### Common Commands

```bash
# Build firmware
mise run build

# Flash to board (put in BOOTSEL mode first)
mise run flash

# Build + Flash in one command
mise run build-flash

# View serial output / debug logs
mise run serial

# Development mode: build, flash, and monitor
mise run dev

# Clean rebuild
mise run rebuild

# See all available commands
mise tasks
```

### Typical Development Workflow

**First time / big changes:**
```bash
# Build and flash
mise run build-flash

# Then view serial output
mise run serial
```

**Quick iteration:**
```bash
# Edit code
vim src/main.c

# Build, flash, and monitor in one command
mise run dev
```

### Flashing to Board

The `mise run flash` command automatically:
- Detects your board in BOOTSEL mode
- Flashes the firmware
- Handles macOS extended attributes issues

**Manual steps (if needed):**
1. Hold BOOTSEL button while plugging in USB
2. Run `mise run flash`

## Project Structure

```
src/
├── main.c                  # Entry point & main loop
├── hal/                    # Hardware abstraction
│   ├── display_driver.c/h  # SPI display driver
│   ├── lvgl_helpers.c/h    # LVGL integration
│   └── (more HAL modules)
└── (future UI components)
```

## Current Status

- ✅ Project structure and build system
- ✅ Display driver HAL stub
- ✅ LVGL integration
- 🚧 Display controller initialization (next)
- ⏳ Touch driver
- ⏳ Software RTC
- ⏳ UI components

## License

MIT License
