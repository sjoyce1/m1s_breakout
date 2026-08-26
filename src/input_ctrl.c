#include "input_ctrl.h"
#include <string.h>

/* bflb_mcu_sdk GPIO and timer headers */
#include "bflb_gpio.h"
#include "bflb_mtimer.h"

static struct bflb_device_s *gpio_dev = NULL;
static uint32_t last_human_activity_ms = 0;
static bool attract_mode_active = false;

/* Debounce filters */
static uint8_t btn_up_history = 0xFF;
static uint8_t btn_down_history = 0xFF;

/* Initialize GPIO buttons with internal pull-up */
void input_ctrl_init(void)
{
    gpio_dev = bflb_device_get_by_name("gpio");

    if (gpio_dev) {
        bflb_gpio_init(gpio_dev, GPIO_BTN_UP, GPIO_INPUT | GPIO_PULLUP | GPIO_SMT_EN);
        bflb_gpio_init(gpio_dev, GPIO_BTN_DOWN, GPIO_INPUT | GPIO_PULLUP | GPIO_SMT_EN);
    }

    last_human_activity_ms = bflb_mtimer_get_time_ms();
    attract_mode_active = false;
}

/* Reset the idle timer */
void input_ctrl_reset_idle(void)
{
    last_human_activity_ms = bflb_mtimer_get_time_ms();
    attract_mode_active = false;
}

/* Check attract mode status */
bool input_is_attract_mode(void)
{
    return attract_mode_active;
}

/* Poll inputs, arbitrate source, and manage attract mode transition */
void input_ctrl_poll(const vision_track_result_t *vision_res, input_state_t *state)
{
    if (!state) return;
    memset(state, 0, sizeof(input_state_t));

    uint32_t now = bflb_mtimer_get_time_ms();

    /* Read raw GPIOs (Active LOW on M1s Dock) */
    bool raw_btn_up = false;
    bool raw_btn_down = false;

    if (gpio_dev) {
        raw_btn_up = !bflb_gpio_read(gpio_dev, GPIO_BTN_UP);
        raw_btn_down = !bflb_gpio_read(gpio_dev, GPIO_BTN_DOWN);
    }

    /* Shift-register 8-sample debounce */
    btn_up_history = (btn_up_history << 1) | (raw_btn_up ? 1 : 0);
    btn_down_history = (btn_down_history << 1) | (raw_btn_down ? 1 : 0);

    bool btn_up = (btn_up_history & 0x0F) == 0x0F;
    bool btn_down = (btn_down_history & 0x0F) == 0x0F;

    state->btn_up_pressed = btn_up;
    state->btn_down_pressed = btn_down;

    /* Detect camera optical motion */
    bool camera_human_action = vision_has_human_gesture(vision_res);

    /* Determine if human interaction happened this tick */
    bool human_active = (btn_up || btn_down || camera_human_action);

    if (human_active) {
        last_human_activity_ms = now;
        attract_mode_active = false;
    }

    uint32_t idle_time = now - last_human_activity_ms;
    state->idle_duration_ms = idle_time;

    /* Attract mode activation check */
    if (idle_time >= ATTRACT_IDLE_TIMEOUT_MS) {
        attract_mode_active = true;
    }

    state->is_attract_mode = attract_mode_active;

    /* Hybrid Input Arbitration */
    if (attract_mode_active) {
        state->active_source = INPUT_SRC_AUTONOMOUS_AI;
    } else if (btn_up || btn_down) {
        /* GPIO buttons take primary priority when actively held */
        state->active_source = INPUT_SRC_GPIO_BUTTONS;
    } else if (vision_res && vision_res->is_detected) {
        /* Camera optical tracking active */
        state->active_source = INPUT_SRC_CAMERA_VISION;
        state->vision_tracked = true;
        state->vision_paddle_y = vision_res->mapped_paddle_y;
    } else {
        state->active_source = INPUT_SRC_NONE;
    }
}
