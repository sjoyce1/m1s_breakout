@echo off
setlocal enabledelayedexpansion

echo =======================================================
echo  Building Sipeed M1s Dock Vertical Breakout Firmware
echo =======================================================

:: Set default SDK path if not already in environment
if "%BL_SDK_BASE%"=="" (
    if exist "..\bflb_mcu_sdk" (
        set "BL_SDK_BASE=..\bflb_mcu_sdk"
    ) else if exist "C:\BouffaloLab\bflb_mcu_sdk" (
        set "BL_SDK_BASE=C:\BouffaloLab\bflb_mcu_sdk"
    ) else (
        echo [WARNING] BL_SDK_BASE environment variable is not set.
        echo Defaulting to 'bflb_mcu_sdk' relative path.
        set "BL_SDK_BASE=bflb_mcu_sdk"
    )
)

echo [INFO] Using SDK: %BL_SDK_BASE%
echo [INFO] Target: BL808 (Core: D0)

:: Run make
make CHIP=bl808 BOARD=bl808_m1s_dock CPU_ID=d0 -j%NUMBER_OF_PROCESSORS%

if %ERRORLEVEL% EQU 0 (
    echo.
    echo =======================================================
    echo  [SUCCESS] Build finished! Binary ready in build/
    echo =======================================================
) else (
    echo.
    echo [ERROR] Build failed with error code %ERRORLEVEL%.
)
