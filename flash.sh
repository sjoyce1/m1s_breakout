#!/usr/bin/env bash
set -e

PORT="${1:-/dev/ttyUSB0}"
FIRMWARE="build/build_out/m1s_breakout_bl808.bin"

if [ ! -f "$FIRMWARE" ]; then
    echo "[ERROR] Firmware binary not found at $FIRMWARE"
    echo "Please run ./build.sh first!"
    exit 1
fi

echo "======================================================="
echo " Flashing $FIRMWARE to M1s Dock on $PORT"
echo "======================================================="

bflb-mcu-tool --chip=bl808 --port="$PORT" --baudrate=2000000 --firmware="$FIRMWARE"

echo "======================================================="
echo " [SUCCESS] Flashing complete! Reset board to run."
echo "======================================================="
