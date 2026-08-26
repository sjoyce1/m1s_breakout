@echo off
setlocal

set "PORT=%1"
if "%PORT%"=="" (
    set "PORT=COM5"
    echo [INFO] No COM port specified. Defaulting to %PORT%.
    echo [USAGE] flash.bat COM^<number^> [baudrate]  (e.g., flash.bat COM5 115200)
)

set "BAUD=%2"
if "%BAUD%"=="" set "BAUD=2000000"

set "CPU_ID=d0"
set "FIRMWARE=build\build_out\m1s_breakout_bl808_d0.bin"
if not exist "%FIRMWARE%" (
    set "CPU_ID=m0"
    set "FIRMWARE=build\build_out\m1s_breakout_bl808_m0.bin"
)
if not exist "%FIRMWARE%" set "FIRMWARE=build\build_out\m1s_breakout_bl808.bin"


if not exist "%FIRMWARE%" (
    echo [ERROR] Firmware binary not found!
    echo Please run build.bat first.
    exit /b 1
)

echo =======================================================
echo  Flashing %FIRMWARE% (Core: %CPU_ID%) to M1s Dock on %PORT% @ %BAUD% baud
echo =======================================================

:: Check for SDK bundled BLFlashCommand first, fallback to bflb-mcu-tool
set "FLASHER=%~dp0bouffalo_sdk\tools\bflb_tools\bouffalo_flash_cube\BLFlashCommand.exe"

if exist "%FLASHER%" (
    if exist "flash_prog_cfg.ini" (
        "%FLASHER%" --interface=uart --baudrate=%BAUD% --port=%PORT% --chipname=bl808 --cpu_id=%CPU_ID% --config=flash_prog_cfg.ini
    ) else (
        "%FLASHER%" --interface=uart --baudrate=%BAUD% --port=%PORT% --chipname=bl808 --cpu_id=%CPU_ID% --firmware="%FIRMWARE%"
    )
) else (
    bflb-mcu-tool --chip=bl808 --port=%PORT% --baudrate=%BAUD% --firmware="%FIRMWARE%"
)



if %ERRORLEVEL% EQU 0 (
    echo.
    echo =======================================================
    echo  [DONE] Operation completed. If flashed successfully,
    echo  press the RST button on the M1s Dock to boot!
    echo =======================================================
) else (
    echo.
    echo [ERROR] Flashing failed.
    echo Tips:
    echo  1. Enter boot mode: Hold BOOT -> Press & Release RST -> Release BOOT.
    echo  2. Try lower baud rate: flash.bat %PORT% 115200 (or 500000 / 1000000)
    echo  3. Ensure cable is on the 'UART' port (not OTG port).
)


