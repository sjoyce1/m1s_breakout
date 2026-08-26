#!/usr/bin/env bash
set -e

echo "======================================================="
echo " Building Sipeed M1s Dock Vertical Breakout Firmware"
echo "======================================================="

export BL_SDK_BASE="${BL_SDK_BASE:-../bflb_mcu_sdk}"
echo "[INFO] Using SDK: $BL_SDK_BASE"
echo "[INFO] Target: BL808 (Core: D0)"

make CHIP=bl808 BOARD=bl808_m1s_dock CPU_ID=d0 -j$(nproc)

echo "======================================================="
echo " [SUCCESS] Build complete! Binary in build/build_out/"
echo "======================================================="
