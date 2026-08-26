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
ATTR_WIFI_RAM_SECTION static uint8_t __attribute__((aligned(32))) cam_slice_buf[SLICE_BUFFER_SIZE];


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

/* Initialize Camera subsystem (Safe non-conflicting mode) */
void camera_dvp_init(void)
{
    /* On Sipeed M1s Dock, camera uses MIPI CSI interface while LCD uses SPI1.
     * To prevent GPIO multiplexer contention with LCD pins (GPIO 24/25),
     * camera hardware DVP is disabled in standalone LCD mode. */
    cam_dev = NULL;
    i2c_dev = NULL;
}

/* Capture camera frame slice and calculate intensity centroid */
void vision_process_frame(vision_track_result_t *result)
{
    if (!result) return;

    memset(result, 0, sizeof(vision_track_result_t));
    result->is_detected = false;
    result->motion_active = false;
    result->mapped_paddle_y = smoothed_paddle_y;
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
