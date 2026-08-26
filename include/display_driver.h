#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "game_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize the SPI controller, GPIO pins, and send ST7789/NV3041 initialization sequence */
void lcd_spi_init(void);

/* Clear the entire screen with a solid color */
void lcd_clear(uint16_t color);

/* Draw a filled rectangle */
void lcd_draw_rect_fill(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

/* Draw a rectangle outline */
void lcd_draw_rect_outline(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

/* Draw a filled circle (for ball) */
void lcd_draw_circle_fill(int16_t cx, int16_t cy, int16_t radius, uint16_t color);

/* Render a single ASCII character */
void lcd_draw_char(int16_t x, int16_t y, char c, uint16_t color, uint16_t bg_color);

/* Render a string */
void lcd_draw_string(int16_t x, int16_t y, const char *str, uint16_t color, uint16_t bg_color);

/* Draw score / number */
void lcd_draw_num(int16_t x, int16_t y, int32_t num, uint16_t color, uint16_t bg_color);

/* Flush a specific rectangular region to the SPI LCD via DMA */
void lcd_flush_rect(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *pixels);

/* Backlight brightness control (0-100) */
void lcd_set_backlight(uint8_t brightness);

#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_DRIVER_H */
