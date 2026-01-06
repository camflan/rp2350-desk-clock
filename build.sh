#!/bin/bash
# Quick build script for RP2350 AMOLED Clock

set -e  # Exit on error

# Set Pico SDK path
export PICO_SDK_PATH=~/pico/pico-sdk

# Build
echo "Building RP2350 AMOLED Clock..."
cmake --build build --parallel $(sysctl -n hw.ncpu)

# Show results
echo ""
echo "✅ Build successful!"
echo ""
echo "Output files:"
ls -lh build/rp2350_amoled_clock.uf2
echo ""
echo "To flash: Copy build/rp2350_amoled_clock.uf2 to your board in BOOTSEL mode"
