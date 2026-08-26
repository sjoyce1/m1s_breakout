#include "camera_vision.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

/* bflb_mcu_sdk drivers & board initialization */
#include "board.h"
#include "bflb_csi.h"
#include "bflb_cam.h"
#include "bflb_i2c.h"
#include "image_sensor.h"
#include "bflb_mtimer.h"

static struct bflb_device_s *i2c_dev = NULL;
static struct bflb_device_s *cam_dev = NULL;
static struct bflb_device_s *csi_dev = NULL;
static bool cam_initialized = false;
static uint16_t img_res_x = 0;
static uint16_t img_res_y = 0;

/* Frame buffer for camera reception */
#define CAM_BUF_SIZE (320 * 240 * 2)
ATTR_WIFI_RAM_SECTION static uint8_t __attribute__((aligned(32))) cam_frame_buf[CAM_BUF_SIZE];

/* Filter state */
static float smoothed_paddle_y = (SCREEN_HEIGHT - PADDLE_HEIGHT) / 2.0f;
static uint32_t last_motion_tick = 0;

/* Initialize MIPI CSI Camera subsystem */
void camera_dvp_init(void)
{
    printf("[CAM] Initializing MIPI CSI Camera interface...\r\n");

    board_csi_gpio_init();

    i2c_dev = bflb_device_get_by_name("i2c0");
    cam_dev = bflb_device_get_by_name("cam0");
    csi_dev = bflb_device_get_by_name("csi");

    if (!i2c_dev || !cam_dev || !csi_dev) {
        printf("[CAM ERROR] Missing CSI/CAM/I2C device handles.\r\n");
        return;
    }

    struct bflb_csi_config_s csi_cfg = {
        .lane_number = CSI_LANE_NUMBER_2,
        .tx_clk_escape = 24000000,
        .data_rate = 520000000,
    };
    bflb_csi_init(csi_dev, &csi_cfg);
    bflb_csi_start(csi_dev);

    struct image_sensor_config_s *sensor_cfg = NULL;
    if (image_sensor_scan(i2c_dev, &sensor_cfg)) {
        printf("[CAM] Found sensor: %s (%dx%d)\r\n", sensor_cfg->name, sensor_cfg->resolution_x, sensor_cfg->resolution_y);
        img_res_x = sensor_cfg->resolution_x;
        img_res_y = sensor_cfg->resolution_y;

        bflb_csi_set_line_threshold(csi_dev, sensor_cfg->resolution_x, sensor_cfg->pixel_clock, 80000000);

        static struct bflb_cam_config_s cam_cfg;
        memcpy(&cam_cfg, sensor_cfg, IMAGE_SENSOR_INFO_COPY_SIZE);
        cam_cfg.with_mjpeg = false;
        cam_cfg.input_source = CAM_INPUT_SOURCE_CSI;
        cam_cfg.output_format = CAM_OUTPUT_FORMAT_AUTO;
        cam_cfg.output_bufaddr = (uint32_t)(uintptr_t)cam_frame_buf;
        cam_cfg.output_bufsize = sizeof(cam_frame_buf);

        bflb_cam_init(cam_dev, &cam_cfg);
        bflb_cam_start(cam_dev);
        cam_initialized = true;
        printf("[CAM] Camera streaming active.\r\n");
    } else {
        printf("[CAM] No sensor detected on I2C (camera not connected or sleep mode).\r\n");
        cam_initialized = false;
    }
}

/* Capture camera frame and calculate intensity centroid */
void vision_process_frame(vision_track_result_t *result)
{
    if (!result) return;
    memset(result, 0, sizeof(vision_track_result_t));
    result->mapped_paddle_y = smoothed_paddle_y;

    if (!cam_initialized || !cam_dev) return;

    if (bflb_cam_get_frame_count(cam_dev) > 0) {
        uint8_t *pic = NULL;
        uint32_t pic_len = bflb_cam_get_frame_info(cam_dev, &pic);
        bflb_cam_pop_one_frame(cam_dev);

        if (!pic || pic_len == 0 || img_res_x == 0 || img_res_y == 0) return;

        /* Optical centroid calculation on luminance (Y components) */
        uint32_t total_weight = 0;
        uint32_t weighted_y = 0;
        int step_y = 4;
        int step_x = 8;

        for (int y = 0; y < img_res_y; y += step_y) {
            uint32_t row_offset = (uint32_t)y * img_res_x * 2;
            for (int x = 0; x < img_res_x; x += step_x) {
                uint8_t luma = pic[row_offset + x * 2];
                if (luma > CAM_LUMA_THRESHOLD) {
                    uint32_t w = luma - CAM_LUMA_THRESHOLD;
                    weighted_y += y * w;
                    total_weight += w;
                }
            }
        }

        if (total_weight > CAM_MIN_PIXEL_MASS) {
            float raw_centroid_y = (float)weighted_y / (float)total_weight;
            float target_paddle_y = (raw_centroid_y / (float)img_res_y) * (SCREEN_HEIGHT - PADDLE_HEIGHT);

            smoothed_paddle_y = smoothed_paddle_y * 0.75f + target_paddle_y * 0.25f;

            result->is_detected = true;
            result->mapped_paddle_y = smoothed_paddle_y;
            result->motion_active = true;
            last_motion_tick = bflb_mtimer_get_time_ms();
        }
    }
}

/* Check if intentional gesture was detected recently */
bool vision_has_human_gesture(const vision_track_result_t *result)
{
    if (!result) return false;
    uint32_t now = bflb_mtimer_get_time_ms();
    return (result->is_detected && ((now - last_motion_tick) < 400));
}

/* Access frame buffer */
const uint8_t* vision_get_slice_buffer(void)
{
    return cam_frame_buf;
}

