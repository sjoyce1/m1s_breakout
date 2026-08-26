@echo off
setlocal enabledelayedexpansion

echo =======================================================
echo  Building Sipeed M1s Dock Vertical Breakout (Core: D0)
echo =======================================================

set "BL_SDK_BASE=%~dp0bouffalo_sdk"
set "PATH=%BL_SDK_BASE%\tools\make;%BL_SDK_BASE%\tools\ninja;%BL_SDK_BASE%\tools\cmake\bin;%~dp0toolchain_gcc_t-head_windows\bin;%PATH%"

echo [INFO] Using SDK: %BL_SDK_BASE%
echo [INFO] Target: BL808 (Core: D0 / C906 64-bit Application Core)

make CHIP=bl808 BOARD=bl808dk CPU_ID=d0 CROSS_COMPILE=riscv64-unknown-elf- -j%NUMBER_OF_PROCESSORS%

if %ERRORLEVEL% EQU 0 (
    echo.
    echo =======================================================
    echo  [SUCCESS] Build finished! Binary ready in build/
    echo =======================================================
) else (
    echo.
    echo [ERROR] Build failed with error code %ERRORLEVEL%.
)



