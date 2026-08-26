#ifndef CAMERA_VISION_H
#define CAMERA_VISION_H

#include <stdint.h>
#include <stdbool.h>
#include "game_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float centroid_x;       /* Normalized X position [0.0f, 1.0f] */
    float centroid_y;       /* Normalized Y position [0.0f, 1.0f] */
    float mapped_paddle_y;  /* Mapped Y position for paddle in screen coordinates */
    uint32_t total_mass;    /* Brightness pixel mass (confidence metric) */
    uint8_t peak_luma;      /* Maximum luminance detected */
    bool is_detected;       /* True if a valid bright centroid / hand is tracked */
    bool motion_active;     /* True if recent significant movement was detected */
} vision_track_result_t;

/* Initialize DVP peripheral, DMA, and camera sensor (GC0328/GC0308) via I2C */
void camera_dvp_init(void);

/* Capture and process camera frame buffer slice for bright centroid */
void vision_process_frame(vision_track_result_t *result);

/* Check if camera detected active human input gesture */
bool vision_has_human_gesture(const vision_track_result_t *result);

/* Get pointer to current frame slice buffer (for debugging or overlays) */
const uint8_t* vision_get_slice_buffer(void);

#ifdef __cplusplus
}
#endif

#endif /* CAMERA_VISION_H */
