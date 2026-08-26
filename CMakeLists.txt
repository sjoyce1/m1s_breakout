cmake_minimum_required(VERSION 3.15)

include(proj.conf)

find_package(bflb_mcu_sdk REQUIRED)

# Project Name
project(m1s_breakout)

# Include directories
sdk_add_include_directories(include)

# Source files
sdk_add_compile_definitions(
    SCREEN_WIDTH=280
    SCREEN_HEIGHT=240
)

sdk_add_sources(
    src/main.c
    src/display_driver.c
    src/camera_vision.c
    src/input_ctrl.c
    src/breakout_game.c
)

sdk_set_main_entry(src/main.c)
