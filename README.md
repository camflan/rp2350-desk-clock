# RP2350 AMOLED Desk Clock

Firmware for the Waveshare RP2350-Touch-AMOLED-1.43 desk clock.

## Prerequisites

```sh
mise run setup   # clone pico-sdk + install ARM toolchain
```

## Build & Flash

```sh
mise run build   # configure + compile
mise run flash   # load via picotool + reboot
mise run serial  # open USB serial at 115200 baud
mise run dev     # build + flash + serial (all-in-one)
```

## First Flash (BOOTSEL)

The very first time (or if the device is unresponsive), you need to enter BOOTSEL mode:

1. Hold the **BOOT** button on the board
2. While holding BOOT, press and release **RESET** (or plug in USB)
3. Release BOOT — the board mounts as a USB mass-storage drive
4. Run `mise run flash` (picotool can find the device in BOOTSEL mode)

After the first flash, `picotool load -f` can reboot into BOOTSEL automatically via USB, so you won't need to hold the button again.
