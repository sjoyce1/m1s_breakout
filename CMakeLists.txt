cmake_minimum_required(VERSION 3.15)

include(proj.conf)

find_package(bouffalo_sdk REQUIRED HINTS $ENV{BL_SDK_BASE})

sdk_add_include_directories(include)

sdk_add_compile_definitions(
    SCREEN_WIDTH=280
    SCREEN_HEIGHT=240
)

target_sources(app PRIVATE
    src/display_driver.c
    src/camera_vision.c
    src/input_ctrl.c
    src/breakout_game.c
)

sdk_set_main_file(src/main.c)

project(m1s_breakout)

