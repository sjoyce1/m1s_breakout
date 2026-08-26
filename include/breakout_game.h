#ifndef BREAKOUT_GAME_H
#define BREAKOUT_GAME_H

#include <stdint.h>
#include <stdbool.h>
#include "game_config.h"
#include "input_ctrl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GAME_STATE_TITLE = 0,
    GAME_STATE_PLAYING,
    GAME_STATE_ROUND_CLEAR,
    GAME_STATE_GAME_OVER
} game_state_t;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
    uint16_t color;
    uint8_t points;
    bool active;
} brick_t;

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    int16_t radius;
    uint16_t color;
    bool in_play;
} ball_t;

typedef struct {
    float y;
    int16_t x;
    int16_t w;
    int16_t h;
    float vy;
    uint16_t color;
    float ai_target_offset; /* Randomized error margin for attract mode */
} paddle_t;

typedef struct {
    game_state_t state;
    uint32_t score;
    uint32_t high_score;
    int8_t lives;
    uint16_t round;
    uint16_t active_bricks_count;
    
    paddle_t paddle;
    paddle_t prev_paddle;
    ball_t ball;
    ball_t prev_ball;
    
    brick_t bricks[BRICK_COLS][BRICK_ROWS];
    
    /* Attract mode state */
    bool is_attract_mode;
    uint32_t attract_ai_timer;
    uint32_t frame_count;
    
    /* Dirty flags for high-performance rendering */
    bool full_redraw_needed;
    bool hud_redraw_needed;
} breakout_game_t;

/* Initialize game state, reset scores, setup brick matrix and paddle */
void breakout_game_init(breakout_game_t *game);

/* Reset round / level with full set of bricks */
void breakout_game_reset_round(breakout_game_t *game);

/* Reset ball on paddle */
void breakout_game_spawn_ball(breakout_game_t *game);

/* Main 60 FPS Game Tick function: updates physics, collisions, input, and attract AI */
void breakout_game_tick(breakout_game_t *game, const input_state_t *input);

/* Render game elements to the 280x240 LCD using dirty rectangles for 60 FPS */
void breakout_game_render(breakout_game_t *game);

#ifdef __cplusplus
}
#endif

#endif /* BREAKOUT_GAME_H */
