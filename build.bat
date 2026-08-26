@echo off
setlocal enabledelayedexpansion

echo =======================================================
echo  Building Sipeed M1s Dock Vertical Breakout Firmware
echo =======================================================

:: Set default SDK path if not already in environment or if invalid
if not exist "%BL_SDK_BASE%\project.build" (
    if exist "%~dp0bouffalo_sdk\project.build" (
        set "BL_SDK_BASE=%~dp0bouffalo_sdk"
    ) else if exist "%~dp0..\bouffalo_sdk\project.build" (
        set "BL_SDK_BASE=%~dp0..\bouffalo_sdk"
    ) else if exist "C:\BouffaloLab\bouffalo_sdk\project.build" (
        set "BL_SDK_BASE=C:\BouffaloLab\bouffalo_sdk"
    ) else (
        echo [WARNING] bouffalo_sdk directory not automatically found.
        echo Please ensure BL_SDK_BASE points to your bouffalo_sdk directory.
    )
)

:: Add SDK bundled build utilities (make, cmake, ninja) to PATH
if exist "%BL_SDK_BASE%\tools\make" set "PATH=%BL_SDK_BASE%\tools\make;%PATH%"
if exist "%BL_SDK_BASE%\tools\ninja" set "PATH=%BL_SDK_BASE%\tools\ninja;%PATH%"
if exist "%BL_SDK_BASE%\tools\cmake\bin" set "PATH=%BL_SDK_BASE%\tools\cmake\bin;%PATH%"

:: Add toolchain to PATH if present alongside SDK or in workspace
if exist "%~dp0toolchain_gcc_t-head_windows\bin" set "PATH=%~dp0toolchain_gcc_t-head_windows\bin;%PATH%"
if exist "%~dp0..\toolchain_gcc_t-head_windows\bin" set "PATH=%~dp0..\toolchain_gcc_t-head_windows\bin;%PATH%"
if exist "C:\BouffaloLab\toolchain_gcc_t-head_windows\bin" set "PATH=C:\BouffaloLab\toolchain_gcc_t-head_windows\bin;%PATH%"

echo [INFO] Using SDK: %BL_SDK_BASE%
echo [INFO] Target: BL808 (Core: M0 / E907 Primary Boot Core)

:: Run make
make CHIP=bl808 BOARD=bl808dk CPU_ID=m0 CROSS_COMPILE=riscv64-unknown-elf- -j%NUMBER_OF_PROCESSORS%




if %ERRORLEVEL% EQU 0 (
    echo.
    echo =======================================================
    echo  [SUCCESS] Build finished! Binary ready in build/
    echo =======================================================
) else (
    echo.
    echo [ERROR] Build failed with error code %ERRORLEVEL%.
)

