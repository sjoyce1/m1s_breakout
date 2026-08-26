#include "camera_vision.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

/* bflb_mcu_sdk camera, i2c and timer drivers */
#include "bflb_cam.h"
#include "bflb_i2c.h"
#include "bflb_gpio.h"
#include "bflb_mtimer.h"

/* GC0328 I2C Address (7-bit 0x21, 8-bit write 0x42) */
#define GC0328_I2C_ADDR      0x21

/* Camera DVP peripheral handle */
static struct bflb_device_s *cam_dev = NULL;
static struct bflb_device_s *i2c_dev = NULL;

/* Frame buffer slice for DVP DMA reception */
/* YUV422 format: 2 bytes per pixel (Y0, U0, Y1, V0) */
#define SLICE_BUFFER_SIZE (CAM_FRAME_WIDTH * CAM_SLICE_HEIGHT * 2)
static uint8_t __attribute__((aligned(32))) cam_slice_buf[SLICE_BUFFER_SIZE];

/* Filter state */
static float smoothed_paddle_y = (SCREEN_HEIGHT - PADDLE_HEIGHT) / 2.0f;
static float prev_raw_y = 0.0f;
static uint32_t last_motion_tick = 0;

/* Internal helper: I2C write byte to sensor register */
static void sensor_write_reg(uint8_t reg, uint8_t val)
{
    if (!i2c_dev) return;
    struct bflb_i2c_msg_s msgs[1];
    uint8_t buf[2] = { reg, val };

    msgs[0].addr = GC0328_I2C_ADDR;
    msgs[0].flags = 0;
    msgs[0].buffer = buf;
    msgs[0].length = 2;

    bflb_i2c_transfer(i2c_dev, msgs, 1);
}

/* Minimal sensor init table for GC0328 (YUV422 QQVGA 160x120 output) */
static const uint8_t gc0328_init_regs[][2] = {
    {0xfe, 0x80}, /* Software reset */
    {0xfe, 0x00}, /* Page 0 */
    {0x14, 0x10}, /* Clock divider */
    {0x24, 0xa2}, /* Output format: YUV422 */
    {0x25, 0x0f}, /* YUYV sequence */
    {0x1a, 0x2a}, /* Analog enable */
    {0x1c, 0x4f},
    {0x1d, 0x13},
    {0xfe, 0x01}, /* Page 1 */
    {0x0a, 0x00}, /* Auto exposure enable */
    {0x40, 0x22}, /* Auto white balance */
    {0xfe, 0x00}, /* Page 0 */
    {0x09, 0x00}, /* Row start */
    {0x0a, 0x00},
    {0x0b, 0x00}, /* Column start */
    {0x0c, 0x00},
    {0x0d, 0x00}, /* Window height (120 lines QQVGA) */
    {0x0e, 0x78},
    {0x0f, 0x00}, /* Window width (160 cols QQVGA) */
    {0x10, 0xa0},
};

/* Initialize DVP Camera, I2C sensor link, and DMA capture */
void camera_dvp_init(void)
{
    i2c_dev = bflb_device_get_by_name("i2c0");
    cam_dev = bflb_device_get_by_name("cam0");

    if (i2c_dev) {
        bflb_i2c_init(i2c_dev, 400000); /* 400 kHz Fast-mode I2C */

        /* Upload sensor registers */
        bflb_mtimer_delay_ms(10);
        for (size_t i = 0; i < sizeof(gc0328_init_regs) / sizeof(gc0328_init_regs[0]); i++) {
            sensor_write_reg(gc0328_init_regs[i][0], gc0328_init_regs[i][1]);
            bflb_mtimer_delay_ms(1);
        }
    }

    if (cam_dev) {
        struct bflb_cam_config_s cam_cfg = {
            .software_mode = CAM_AUTO_MODE,
            .frame_mode = CAM_FRAME_INTERLEAVE,
            .yuv_format = CAM_YUV_FORMAT_YUV422_YUYV,
            .with_header = 0,
            .input_source = CAM_INPUT_DVP_NORMAL,
            .input_dvp_data_format = CAM_DATA_FORMAT_YUV422_8BIT,
            .resolution_x = CAM_FRAME_WIDTH,
            .resolution_y = CAM_SLICE_HEIGHT,
            .h_blank = 0,
            .v_blank = 0,
        };

        bflb_cam_init(cam_dev, &cam_cfg);
        bflb_cam_crop(cam_dev, 0, CAM_FRAME_WIDTH, CAM_SLICE_Y_START, CAM_SLICE_HEIGHT);
        bflb_cam_start(cam_dev);
    }
}

/* Capture camera frame slice and calculate intensity centroid */
void vision_process_frame(vision_track_result_t *result)
{
    if (!result) return;

    memset(result, 0, sizeof(vision_track_result_t));

    /* In DVP DMA capture mode, fetch new buffer */
    if (cam_dev) {
        uint8_t *frame_ptr = NULL;
        uint32_t frame_len = 0;
        if (bflb_cam_get_frame_info(cam_dev, &frame_ptr, &frame_len) == 0 && frame_ptr) {
            uint32_t copy_len = (frame_len > SLICE_BUFFER_SIZE) ? SLICE_BUFFER_SIZE : frame_len;
            memcpy(cam_slice_buf, frame_ptr, copy_len);
            bflb_cam_pop_one_frame(cam_dev);
        }
    }

    uint64_t sum_x_weight = 0;
    uint64_t sum_y_weight = 0;
    uint32_t total_mass = 0;
    uint8_t max_luma = 0;

    /* Scan the slice buffer for high luminance pixels (Y channel in YUV422) */
    for (int y = 0; y < CAM_SLICE_HEIGHT; y++) {
        int row_offset = y * CAM_FRAME_WIDTH * 2;
        for (int x = 0; x < CAM_FRAME_WIDTH; x++) {
            /* In YUYV, Y is at even byte indices (0, 2, 4...) */
            uint8_t luma = cam_slice_buf[row_offset + (x * 2)];

            if (luma > max_luma) {
                max_luma = luma;
            }

            if (luma >= CAM_LUMA_THRESHOLD) {
                uint32_t weight = (uint32_t)(luma - CAM_LUMA_THRESHOLD + 1);
                sum_x_weight += (uint64_t)x * weight;
                sum_y_weight += (uint64_t)y * weight;
                total_mass += weight;
            }
        }
    }

    result->peak_luma = max_luma;
    result->total_mass = total_mass;

    /* Determine if we have a solid tracking signal */
    if (total_mass >= CAM_MIN_PIXEL_MASS) {
        float raw_cx = (float)sum_x_weight / (float)total_mass;
        float raw_cy = (float)sum_y_weight / (float)total_mass;

        result->centroid_x = raw_cx / (float)CAM_FRAME_WIDTH;
        result->centroid_y = raw_cy / (float)CAM_SLICE_HEIGHT;
        result->is_detected = true;

        /* Map camera Y to paddle Y screen coordinates */
        float target_paddle_y = result->centroid_y * (float)(SCREEN_HEIGHT - PADDLE_HEIGHT);

        /* Exponential Moving Average (EMA) smoothing (alpha = 0.35) */
        const float alpha = 0.35f;
        smoothed_paddle_y = alpha * target_paddle_y + (1.0f - alpha) * smoothed_paddle_y;
        result->mapped_paddle_y = smoothed_paddle_y;

        /* Check for active delta motion (human gesture detection) */
        float delta_y = fabsf(raw_cy - prev_raw_y);
        if (delta_y > 1.8f) {
            result->motion_active = true;
            last_motion_tick = bflb_mtimer_get_time_ms();
        }
        prev_raw_y = raw_cy;
    } else {
        result->is_detected = false;
        result->motion_active = false;
        result->mapped_paddle_y = smoothed_paddle_y;
    }
}

/* Check if intentional gesture was detected recently */
bool vision_has_human_gesture(const vision_track_result_t *result)
{
    if (!result) return false;
    uint32_t now = bflb_mtimer_get_time_ms();
    return (result->is_detected && ((now - last_motion_tick) < 400));
}

/* Access frame slice buffer */
const uint8_t* vision_get_slice_buffer(void)
{
    return cam_slice_buf;
}
