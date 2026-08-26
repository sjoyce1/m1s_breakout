#ifndef INPUT_CTRL_H
#define INPUT_CTRL_H

#include <stdint.h>
#include <stdbool.h>
#include "game_config.h"
#include "camera_vision.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    INPUT_SRC_NONE = 0,
    INPUT_SRC_GPIO_BUTTONS,
    INPUT_SRC_CAMERA_VISION,
    INPUT_SRC_AUTONOMOUS_AI
} input_source_t;

typedef struct {
    bool btn_up_pressed;
    bool btn_down_pressed;
    bool vision_tracked;
    float vision_paddle_y;
    input_source_t active_source;
    bool is_attract_mode;
    uint32_t idle_duration_ms;
} input_state_t;

/* Initialize GPIO buttons (GPIO 22 and GPIO 23) with internal pull-up */
void input_ctrl_init(void);

/* Poll and update all input states, compute idle time and mode switching */
void input_ctrl_poll(const vision_track_result_t *vision_res, input_state_t *state);

/* Force reset idle timer (e.g. on new game start or explicit player action) */
void input_ctrl_reset_idle(void);

/* Check if attract mode is currently active */
bool input_is_attract_mode(void);

#ifdef __cplusplus
}
#endif

#endif /* INPUT_CTRL_H */
