#include "breakout_game.h"
#include "display_driver.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

/* Column colors & point mappings (from left to right) */
static const struct {
    uint16_t color;
    uint8_t points;
} col_specs[BRICK_COLS] = {
    {COLOR_RED,    50},
    {COLOR_ORANGE, 40},
    {COLOR_YELLOW, 30},
    {COLOR_GREEN,  20},
    {COLOR_CYAN,   10}
};

/* Random float generator [-1.0, 1.0] */
static inline float rand_unit_float(void)
{
    return ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
}

/* Initialize game state */
void breakout_game_init(breakout_game_t *game)
{
    if (!game) return;

    game->state = GAME_STATE_PLAYING;
    game->score = 0;
    game->high_score = 1000;
    game->lives = 3;
    game->round = 1;
    game->is_attract_mode = false;
    game->frame_count = 0;
    game->attract_ai_timer = 0;

    /* Initialize paddle */
    game->paddle.x = PADDLE_X_POS;
    game->paddle.y = (SCREEN_HEIGHT - PADDLE_HEIGHT) / 2.0f;
    game->paddle.w = PADDLE_WIDTH;
    game->paddle.h = PADDLE_HEIGHT;
    game->paddle.vy = 0.0f;
    game->paddle.color = COLOR_WHITE;
    game->paddle.ai_target_offset = 0.0f;
    game->prev_paddle = game->paddle;

    /* Initialize ball */
    game->ball.radius = BALL_RADIUS;
    game->ball.color = COLOR_YELLOW;
    breakout_game_spawn_ball(game);
    game->prev_ball = game->ball;

    /* Setup bricks */
    breakout_game_reset_round(game);

    game->full_redraw_needed = true;
    game->hud_redraw_needed = true;
}

/* Reset level bricks */
void breakout_game_reset_round(breakout_game_t *game)
{
    game->active_bricks_count = 0;

    for (int col = 0; col < BRICK_COLS; col++) {
        for (int row = 0; row < BRICK_ROWS; row++) {
            brick_t *b = &game->bricks[col][row];
            b->x = BRICK_START_X + col * (BRICK_WIDTH + BRICK_GAP_X);
            b->y = BRICK_START_Y + row * (BRICK_HEIGHT + BRICK_GAP_Y);
            b->w = BRICK_WIDTH;
            b->h = BRICK_HEIGHT;
            b->color = col_specs[col].color;
            b->points = col_specs[col].points;
            b->active = true;
            game->active_bricks_count++;
        }
    }

    game->full_redraw_needed = true;
}

/* Respawn ball */
void breakout_game_spawn_ball(breakout_game_t *game)
{
    game->ball.x = game->paddle.x - BALL_RADIUS - 2;
    game->ball.y = game->paddle.y + (game->paddle.h / 2.0f);
    
    /* Launch ball leftwards towards bricks with initial angle */
    game->ball.vx = -BALL_BASE_SPEED_X;
    game->ball.vy = rand_unit_float() * 1.5f;
    game->ball.in_play = true;
}

/* Main 60 FPS Game Tick */
void breakout_game_tick(breakout_game_t *game, const input_state_t *input)
{
    if (!game || !input) return;

    game->frame_count++;
    game->prev_paddle = game->paddle;
    game->prev_ball = game->ball;
    game->is_attract_mode = input->is_attract_mode;

    /* -------------------------------------------------------------
     * 1. Paddle Movement Handling (Hybrid: GPIO / Camera / AI)
     * ------------------------------------------------------------- */
    if (input->is_attract_mode) {
        /* ==================== AUTONOMOUS ATTRACT MODE ==================== */
        game->attract_ai_timer++;

        /* Periodically perturb AI offset every 45 frames (~0.75s) to simulate natural tracking */
        if (game->attract_ai_timer % 45 == 0) {
            game->paddle.ai_target_offset = rand_unit_float() * 10.0f;
        }

        /* Target Y: intercept ball Y position */
        float target_y = game->ball.y - (game->paddle.h / 2.0f) + game->paddle.ai_target_offset;

        /* Damped tracking (lerp factor 0.14) */
        float dy = target_y - game->paddle.y;
        float max_step = 4.2f; /* Max tracking speed per frame */
        if (dy > max_step) dy = max_step;
        if (dy < -max_step) dy = -max_step;

        game->paddle.y += dy * 0.75f;

    } else if (input->active_source == INPUT_SRC_GPIO_BUTTONS) {
        /* ==================== MANUAL GPIO BUTTON CONTROLS ==================== */
        if (input->btn_up_pressed) {
            game->paddle.y -= PADDLE_MOVE_SPEED;
        }
        if (input->btn_down_pressed) {
            game->paddle.y += PADDLE_MOVE_SPEED;
        }

    } else if (input->active_source == INPUT_SRC_CAMERA_VISION && input->vision_tracked) {
        /* ==================== DVP CAMERA OPTICAL TRACKING ==================== */
        /* Follow smoothed mapped optical centroid */
        float diff = input->vision_paddle_y - game->paddle.y;
        game->paddle.y += diff * 0.40f;
    }

    /* Clamp paddle within screen bounds */
    if (game->paddle.y < 0) {
        game->paddle.y = 0;
    }
    if (game->paddle.y > (SCREEN_HEIGHT - game->paddle.h)) {
        game->paddle.y = (float)(SCREEN_HEIGHT - game->paddle.h);
    }

    /* -------------------------------------------------------------
     * 2. Ball Physics & Motion Update
     * ------------------------------------------------------------- */
    if (game->ball.in_play) {
        game->ball.x += game->ball.vx;
        game->ball.y += game->ball.vy;

        /* Wall Collisions */
        /* Top Wall */
        if (game->ball.y - game->ball.radius <= 0) {
            game->ball.y = (float)game->ball.radius;
            game->ball.vy = -game->ball.vy;
        }
        /* Bottom Wall */
        if (game->ball.y + game->ball.radius >= SCREEN_HEIGHT - 1) {
            game->ball.y = (float)(SCREEN_HEIGHT - 1 - game->ball.radius);
            game->ball.vy = -game->ball.vy;
        }
        /* Left Wall (behind bricks) */
        if (game->ball.x - game->ball.radius <= 0) {
            game->ball.x = (float)game->ball.radius;
            game->ball.vx = fabsf(game->ball.vx); /* Rebound right */
        }

        /* Paddle Collision (Paddle on the right side) */
        float paddle_left = (float)game->paddle.x;
        float paddle_right = paddle_left + game->paddle.w;
        float paddle_top = game->paddle.y;
        float paddle_bottom = paddle_top + game->paddle.h;

        if ((game->ball.x + game->ball.radius >= paddle_left) &&
            (game->ball.x - game->ball.radius <= paddle_right) &&
            (game->ball.y >= paddle_top - 2) &&
            (game->ball.y <= paddle_bottom + 2) &&
            (game->ball.vx > 0)) /* Moving towards paddle */
        {
            /* Bounce back to the left */
            game->ball.x = paddle_left - game->ball.radius - 0.5f;
            
            /* Increase speed incrementally */
            float speed = fabsf(game->ball.vx) + BALL_SPEED_INCREMENT;
            game->ball.vx = -speed;

            /* Calculate spin / vertical angle based on hit location */
            float paddle_center_y = paddle_top + (game->paddle.h / 2.0f);
            float relative_hit = (game->ball.y - paddle_center_y) / (game->paddle.h / 2.0f);
            if (relative_hit > 1.0f) relative_hit = 1.0f;
            if (relative_hit < -1.0f) relative_hit = -1.0f;

            game->ball.vy = relative_hit * BALL_MAX_SPEED_Y;
        }

        /* Ball Lost: Passed player's paddle to the right */
        if (game->ball.x - game->ball.radius > SCREEN_WIDTH) {
            if (game->is_attract_mode) {
                /* In attract mode, automatically keep rolling */
                breakout_game_spawn_ball(game);
            } else {
                game->lives--;
                game->hud_redraw_needed = true;
                if (game->lives <= 0) {
                    game->state = GAME_STATE_GAME_OVER;
                    if (game->score > game->high_score) {
                        game->high_score = game->score;
                    }
                } else {
                    breakout_game_spawn_ball(game);
                }
            }
        }

        /* -------------------------------------------------------------
         * 3. Brick Collision Detection
         * ------------------------------------------------------------- */
        for (int col = 0; col < BRICK_COLS; col++) {
            for (int row = 0; row < BRICK_ROWS; row++) {
                brick_t *b = &game->bricks[col][row];
                if (!b->active) continue;

                /* Axis-Aligned Bounding Box (AABB) intersection */
                if ((game->ball.x + game->ball.radius >= b->x) &&
                    (game->ball.x - game->ball.radius <= b->x + b->w) &&
                    (game->ball.y + game->ball.radius >= b->y) &&
                    (game->ball.y - game->ball.radius <= b->y + b->h))
                {
                    /* Deactivate brick */
                    b->active = false;
                    game->active_bricks_count--;
                    game->score += b->points;
                    game->hud_redraw_needed = true;

                    /* Erase brick on screen */
                    lcd_draw_rect_fill(b->x, b->y, b->w, b->h, COLOR_BLACK);

                    /* Determine rebound axis */
                    float overlap_left   = (game->ball.x + game->ball.radius) - b->x;
                    float overlap_right  = (b->x + b->w) - (game->ball.x - game->ball.radius);
                    float overlap_top    = (game->ball.y + game->ball.radius) - b->y;
                    float overlap_bottom = (b->y + b->h) - (game->ball.y - game->ball.radius);

                    float min_overlap_x = (overlap_left < overlap_right) ? overlap_left : overlap_right;
                    float min_overlap_y = (overlap_top < overlap_bottom) ? overlap_top : overlap_bottom;

                    if (min_overlap_x < min_overlap_y) {
                        game->ball.vx = -game->ball.vx;
                    } else {
                        game->ball.vy = -game->ball.vy;
                    }

                    /* Check if all bricks cleared */
                    if (game->active_bricks_count == 0) {
                        game->round++;
                        game->score += 500; /* Clear bonus */
                        breakout_game_reset_round(game);
                        breakout_game_spawn_ball(game);
                        game->hud_redraw_needed = true;
                    }

                    goto collision_done; /* One brick collision per physics step */
                }
            }
        }
    collision_done:;
    }
}

/* Render Game elements (Optimized 60 FPS Dirty Rect rendering) */
void breakout_game_render(breakout_game_t *game)
{
    if (!game) return;

    /* Full screen redraw when resetting rounds / game over */
    if (game->full_redraw_needed) {
        lcd_clear(COLOR_BLACK);

        /* Draw Playfield Border / Court Lines */
        lcd_draw_rect_outline(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_DARK_GRAY);

        /* Draw all active bricks */
        for (int col = 0; col < BRICK_COLS; col++) {
            for (int row = 0; row < BRICK_ROWS; row++) {
                brick_t *b = &game->bricks[col][row];
                if (b->active) {
                    lcd_draw_rect_fill(b->x, b->y, b->w, b->h, b->color);
                    lcd_draw_rect_outline(b->x, b->y, b->w, b->h, COLOR_BLACK);
                }
            }
        }

        game->full_redraw_needed = false;
        game->hud_redraw_needed = true;
    }

    /* -------------------------------------------------------------
     * 1. Redraw Paddle (Dirty Rect: erase old, draw new)
     * ------------------------------------------------------------- */
    int16_t old_py = (int16_t)game->prev_paddle.y;
    int16_t new_py = (int16_t)game->paddle.y;

    if (old_py != new_py) {
        /* Erase previous paddle bbox */
        lcd_draw_rect_fill(game->prev_paddle.x, old_py, game->prev_paddle.w, game->prev_paddle.h, COLOR_BLACK);
    }
    /* Draw current paddle (Neon Green in attract mode, White in manual) */
    uint16_t paddle_col = game->is_attract_mode ? COLOR_NEON_GREEN : COLOR_WHITE;
    lcd_draw_rect_fill(game->paddle.x, new_py, game->paddle.w, game->paddle.h, paddle_col);

    /* -------------------------------------------------------------
     * 2. Redraw Ball (Dirty Rect: erase old, draw new)
     * ------------------------------------------------------------- */
    int16_t old_bx = (int16_t)game->prev_ball.x;
    int16_t old_by = (int16_t)game->prev_ball.y;
    int16_t new_bx = (int16_t)game->ball.x;
    int16_t new_by = (int16_t)game->ball.y;

    if (old_bx != new_bx || old_by != new_by) {
        /* Erase old ball */
        lcd_draw_circle_fill(old_bx, old_by, game->ball.radius + 1, COLOR_BLACK);
    }
    /* Draw new ball */
    lcd_draw_circle_fill(new_bx, new_by, game->ball.radius, game->ball.color);

    /* -------------------------------------------------------------
     * 3. HUD Redraw (Score, Lives, Attract Mode indicator)
     * ------------------------------------------------------------- */
    if (game->hud_redraw_needed || (game->frame_count % 30 == 0)) {
        /* Score HUD on top right */
        lcd_draw_string(SCREEN_WIDTH - 96, 4, "SCR:", COLOR_LIGHT_GRAY, COLOR_BLACK);
        lcd_draw_num(SCREEN_WIDTH - 66, 4, game->score, COLOR_YELLOW, COLOR_BLACK);

        /* Lives indicator */
        if (!game->is_attract_mode) {
            lcd_draw_string(SCREEN_WIDTH - 96, 14, "LIV:", COLOR_LIGHT_GRAY, COLOR_BLACK);
            lcd_draw_num(SCREEN_WIDTH - 66, 14, game->lives, COLOR_CYAN, COLOR_BLACK);
        }

        /* Attract Mode Flashing Banner */
        if (game->is_attract_mode) {
            bool flash = (game->frame_count / 30) % 2 == 0;
            uint16_t badge_color = flash ? COLOR_MAGENTA : COLOR_YELLOW;
            lcd_draw_string(90, 4, "* AUTO-PILOT *", badge_color, COLOR_BLACK);
            lcd_draw_string(72, SCREEN_HEIGHT - 12, "TOUCH BTN/CAM TO PLAY", COLOR_WHITE, COLOR_BLACK);
        } else {
            /* Erase attract banner when in human control */
            lcd_draw_rect_fill(90, 4, 90, 8, COLOR_BLACK);
            lcd_draw_rect_fill(72, SCREEN_HEIGHT - 12, 140, 8, COLOR_BLACK);
        }

        game->hud_redraw_needed = false;
    }

    /* Game Over Overlay */
    if (game->state == GAME_STATE_GAME_OVER) {
        lcd_draw_rect_fill(60, 90, 160, 50, COLOR_DARK_GRAY);
        lcd_draw_rect_outline(60, 90, 160, 50, COLOR_RED);
        lcd_draw_string(98, 100, "GAME OVER", COLOR_RED, COLOR_DARK_GRAY);
        lcd_draw_string(76, 116, "PRESS BTN TO RETRY", COLOR_WHITE, COLOR_DARK_GRAY);
    }
}
