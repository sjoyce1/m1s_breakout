#ifndef GAME_CONFIG_H
#define GAME_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Display & Framebuffer Configuration (280x240 SPI LCD)
 * ============================================================================ */
#define SCREEN_WIDTH         280
#define SCREEN_HEIGHT        240
#define COLOR_RGB565(r, g, b) ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | (((b) & 0xF8) >> 3))

/* Standard RGB565 Colors */
#define COLOR_BLACK          0x0000
#define COLOR_WHITE          0xFFFF
#define COLOR_RED            0xF800
#define COLOR_GREEN          0x07E0
#define COLOR_BLUE           0x001F
#define COLOR_YELLOW         0xFFE0
#define COLOR_CYAN           0x07FF
#define COLOR_MAGENTA        0xF81F
#define COLOR_ORANGE         0xFC00
#define COLOR_DARK_GRAY      0x2104
#define COLOR_LIGHT_GRAY     0x8410
#define COLOR_NEON_GREEN     0x37E6
#define COLOR_PURPLE         0x780F

/* ============================================================================
 * GPIO & Hardware Pin Definitions (Sipeed M1s Dock / BL808)
 * ============================================================================ */
#define GPIO_BTN_UP          22   /* Onboard Button / Navigation Up   */
#define GPIO_BTN_DOWN        23   /* Onboard Button / Navigation Down */

/* LCD SPI Pins (Default M1s Dock Hardware SPI0 / SPI1) */
#define LCD_SPI_MOSI_PIN     3
#define LCD_SPI_SCLK_PIN     2
#define LCD_SPI_CS_PIN       12
#define LCD_SPI_DC_PIN       13
#define LCD_SPI_RESET_PIN    11
#define LCD_SPI_BACKLIGHT_PIN 14

/* DVP Camera Sensor Configuration (GC0328 / GC0308) */
#define CAM_FRAME_WIDTH      160
#define CAM_FRAME_HEIGHT     120
#define CAM_SLICE_Y_START    20
#define CAM_SLICE_HEIGHT     80
#define CAM_LUMA_THRESHOLD   190  /* Brightness threshold for tracking (0-255) */
#define CAM_MIN_PIXEL_MASS   60   /* Minimum aggregate bright pixels to register tracking */

/* ============================================================================
 * Game Physics & Layout Parameters
 * ============================================================================ */
/* Paddle Parameters (Vertical on the Right) */
#define PADDLE_WIDTH         6
#define PADDLE_HEIGHT        46
#define PADDLE_X_POS         (SCREEN_WIDTH - 14)
#define PADDLE_MOVE_SPEED    5.0f

/* Ball Parameters */
#define BALL_RADIUS          3
#define BALL_BASE_SPEED_X    3.2f
#define BALL_MAX_SPEED_Y     4.0f
#define BALL_SPEED_INCREMENT 0.08f

/* Brick Layout (Vertical Columns on the Left) */
#define BRICK_COLS           5    /* Columns across X (Left side) */
#define BRICK_ROWS           10   /* Rows along Y (Vertical) */
#define BRICK_WIDTH          9
#define BRICK_HEIGHT         19
#define BRICK_GAP_X          3
#define BRICK_GAP_Y          3
#define BRICK_START_X        14
#define BRICK_START_Y        12

/* Timing & Idle Attract Mode Parameters */
#define TARGET_FPS           60
#define FRAME_TIME_US        (1000000 / TARGET_FPS)
#define ATTRACT_IDLE_TIMEOUT_MS 10000  /* 10 seconds of inactivity -> Auto-pilot */

#ifdef __cplusplus
}
#endif

#endif /* GAME_CONFIG_H */
