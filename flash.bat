@echo off
setlocal

set "PORT=%1"
if "%PORT%"=="" (
    set "PORT=COM3"
    echo [INFO] No COM port specified. Defaulting to %PORT%.
    echo [USAGE] flash.bat COM^<number^>  (e.g., flash.bat COM5)
)

set "FIRMWARE=build\build_out\m1s_breakout_bl808.bin"

if not exist "%FIRMWARE%" (
    echo [ERROR] Firmware binary not found at %FIRMWARE%
    echo Please run build.bat first!
    exit /b 1
)

echo =======================================================
echo  Flashing %FIRMWARE% to M1s Dock on %PORT%
echo =======================================================

bflb-mcu-tool --chip=bl808 --port=%PORT% --baudrate=2000000 --firmware=%FIRMWARE%

if %ERRORLEVEL% EQU 0 (
    echo.
    echo =======================================================
    echo  [SUCCESS] Flashing complete! Reset board to run.
    echo =======================================================
) else (
    echo.
    echo [ERROR] Flashing failed. Please ensure M1s Dock is connected and bootloader mode is active.
)
