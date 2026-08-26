#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* Bouffalo Lab BL808 MCU SDK drivers */
#include "bflb_mtimer.h"
#include "board.h"

/* Project Game Modules */
#include "game_config.h"
#include "display_driver.h"
#include "camera_vision.h"
#include "input_ctrl.h"
#include "breakout_game.h"

/* Global Game Instance */
static breakout_game_t g_game;

/**
 * @brief Application Entry Point for Sipeed M1s Dock (BL808)
 */
int main(void)
{
    /* 1. Initialize Board Peripherals & UART Console */
    board_init();
    printf("\r\n========================================\r\n");
    printf(" Sipeed M1s Dock - Vertical Breakout 280x240\r\n");
    printf(" MaixHAL / bflb_mcu_sdk Framework\r\n");
    printf("========================================\r\n");

    /* 2. Initialize Hardware Subsystems */
    printf("[INIT] Initializing GPIO buttons (GPIO 22 & 23)...\r\n");
    input_ctrl_init();

    printf("[INIT] Initializing 280x240 SPI LCD (ST7789)...\r\n");
    lcd_spi_init();

    printf("[INIT] Initializing DVP Camera (GC0328) & DMA Engine...\r\n");
    camera_dvp_init();

    printf("[INIT] Initializing Breakout Game Engine...\r\n");
    breakout_game_init(&g_game);

    printf("[GAME] System ready! Entering 60 FPS main loop.\r\n");

    /* Main Loop State */
    vision_track_result_t vision_res;
    input_state_t input_state;
    uint64_t frame_start_us;
    uint64_t frame_elapsed_us;

    /* 3. 60 FPS Master Game Loop */
    while (1) {
        frame_start_us = bflb_mtimer_get_time_us();

        /* Step A: Capture & process camera slice for bright centroid */
        vision_process_frame(&vision_res);

        /* Step B: Poll GPIO buttons & arbitrate hybrid input */
        input_ctrl_poll(&vision_res, &input_state);

        /* Step C: Handle Game Over retry trigger */
        if (g_game.state == GAME_STATE_GAME_OVER) {
            if (input_state.btn_up_pressed || input_state.btn_down_pressed || 
                input_state.is_attract_mode) 
            {
                breakout_game_init(&g_game);
                input_ctrl_reset_idle();
            }
        }

        /* Step D: 60 FPS Game Tick (Physics, Collisions, Attract AI) */
        breakout_game_tick(&g_game, &input_state);

        /* Step E: Optimized Dirty-Rectangle LCD Render */
        breakout_game_render(&g_game);

        /* Step F: Serial Telemetry Log (1 Hz) */
        if (g_game.frame_count % 60 == 0) {
            printf("[GAME] Frame %ld | Score: %d | Attract: %d\r\n", 
                   (long)g_game.frame_count, g_game.score, g_game.is_attract_mode);
        }

        /* Step G: 60 FPS Frame Rate Throttling / Precision Sync */
        frame_elapsed_us = bflb_mtimer_get_time_us() - frame_start_us;
        if (frame_elapsed_us < FRAME_TIME_US) {
            bflb_mtimer_delay_us(FRAME_TIME_US - frame_elapsed_us);
        }
    }


    return 0;
}
