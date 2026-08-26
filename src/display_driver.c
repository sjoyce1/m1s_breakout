#include "display_driver.h"
#include <string.h>
#include <stdio.h>

/* bflb_mcu_sdk peripheral drivers */
#include "bflb_gpio.h"
#include "bflb_spi.h"
#include "bflb_dma.h"
#include "bflb_mtimer.h"

/* LCD Controller Registers (ST7789V / NV3041A) */
#define ST7789_NOP        0x00
#define ST7789_SWRESET    0x01
#define ST7789_SLPIN      0x10
#define ST7789_SLPOUT     0x11
#define ST7789_NORON      0x13
#define ST7789_INVOFF     0x20
#define ST7789_INVON      0x21
#define ST7789_DISPOFF    0x28
#define ST7789_DISPON     0x29
#define ST7789_CASET      0x2A
#define ST7789_RASET      0x2B
#define ST7789_RAMWR      0x2C
#define ST7789_MADCTL     0x36
#define ST7789_COLMOD     0x3A

/* Global peripheral device handles */
static struct bflb_device_s *gpio_dev = NULL;
static struct bflb_device_s *spi_dev = NULL;

/* Line buffer for fast SPI batch transfers (280 pixels * 2 bytes = 560 bytes) */
static uint16_t line_buffer[SCREEN_WIDTH];

/* Basic 5x7 ASCII font table (ASCII 32 ' ' to 126 '~') */
static const uint8_t font5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, /*   */
    {0x00, 0x00, 0x5F, 0x00, 0x00}, /* ! */
    {0x00, 0x07, 0x00, 0x07, 0x00}, /* " */
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, /* # */
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, /* $ */
    {0x23, 0x13, 0x08, 0x64, 0x62}, /* % */
    {0x36, 0x49, 0x55, 0x22, 0x50}, /* & */
    {0x00, 0x05, 0x03, 0x00, 0x00}, /* ' */
    {0x00, 0x1C, 0x22, 0x41, 0x00}, /* ( */
    {0x00, 0x41, 0x22, 0x1C, 0x00}, /* ) */
    {0x08, 0x2A, 0x1C, 0x2A, 0x08}, /* * */
    {0x08, 0x08, 0x3E, 0x08, 0x08}, /* + */
    {0x00, 0x50, 0x30, 0x00, 0x00}, /* , */
    {0x08, 0x08, 0x08, 0x08, 0x08}, /* - */
    {0x00, 0x60, 0x60, 0x00, 0x00}, /* . */
    {0x20, 0x10, 0x08, 0x04, 0x02}, /* / */
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, /* 0 */
    {0x00, 0x42, 0x7F, 0x40, 0x00}, /* 1 */
    {0x42, 0x61, 0x51, 0x49, 0x46}, /* 2 */
    {0x21, 0x41, 0x45, 0x4B, 0x31}, /* 3 */
    {0x18, 0x14, 0x12, 0x7F, 0x10}, /* 4 */
    {0x27, 0x45, 0x45, 0x45, 0x39}, /* 5 */
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, /* 6 */
    {0x01, 0x71, 0x09, 0x05, 0x03}, /* 7 */
    {0x36, 0x49, 0x49, 0x49, 0x36}, /* 8 */
    {0x06, 0x49, 0x49, 0x29, 0x1E}, /* 9 */
    {0x00, 0x36, 0x36, 0x00, 0x00}, /* : */
    {0x00, 0x56, 0x36, 0x00, 0x00}, /* ; */
    {0x00, 0x08, 0x14, 0x22, 0x41}, /* < */
    {0x14, 0x14, 0x14, 0x14, 0x14}, /* = */
    {0x41, 0x22, 0x14, 0x08, 0x00}, /* > */
    {0x02, 0x01, 0x51, 0x09, 0x06}, /* ? */
    {0x32, 0x49, 0x79, 0x41, 0x3E}, /* @ */
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, /* A */
    {0x7F, 0x49, 0x49, 0x49, 0x36}, /* B */
    {0x3E, 0x41, 0x41, 0x41, 0x22}, /* C */
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, /* D */
    {0x7F, 0x49, 0x49, 0x49, 0x41}, /* E */
    {0x7F, 0x09, 0x09, 0x01, 0x01}, /* F */
    {0x3E, 0x41, 0x41, 0x51, 0x32}, /* G */
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, /* H */
    {0x00, 0x41, 0x7F, 0x41, 0x00}, /* I */
    {0x20, 0x40, 0x41, 0x3F, 0x01}, /* J */
    {0x7F, 0x08, 0x14, 0x22, 0x41}, /* K */
    {0x7F, 0x40, 0x40, 0x40, 0x40}, /* L */
    {0x7F, 0x02, 0x04, 0x02, 0x7F}, /* M */
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, /* N */
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, /* O */
    {0x7F, 0x09, 0x09, 0x09, 0x06}, /* P */
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, /* Q */
    {0x7F, 0x09, 0x19, 0x29, 0x46}, /* R */
    {0x46, 0x49, 0x49, 0x49, 0x31}, /* S */
    {0x01, 0x01, 0x7F, 0x01, 0x01}, /* T */
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, /* U */
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, /* V */
    {0x7F, 0x20, 0x18, 0x20, 0x7F}, /* W */
    {0x63, 0x14, 0x08, 0x14, 0x63}, /* X */
    {0x03, 0x04, 0x78, 0x04, 0x03}, /* Y */
    {0x61, 0x51, 0x49, 0x45, 0x43}, /* Z */
};

/* Internal Helper: Send Command to LCD */
static inline void lcd_write_cmd(uint8_t cmd)
{
    bflb_gpio_reset(gpio_dev, LCD_SPI_DC_PIN);
    bflb_spi_poll_exchange(spi_dev, &cmd, NULL, 1);
}

/* Internal Helper: Send multiple bytes to LCD */
static inline void lcd_write_bytes(const uint8_t *data, size_t len)
{
    bflb_gpio_set(gpio_dev, LCD_SPI_DC_PIN);
    bflb_spi_poll_exchange(spi_dev, (void *)data, NULL, len);
}

/* Internal Helper: Send 8-bit Data to LCD */
static inline void lcd_write_data8(uint8_t data)
{
    bflb_gpio_set(gpio_dev, LCD_SPI_DC_PIN);
    bflb_spi_poll_exchange(spi_dev, &data, NULL, 1);
}

/* Set Drawing Window / Viewport */
static void lcd_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    /* Physical hardware panel X offset (+20) for 280x240 landscape mode */
    x1 += 20;
    x2 += 20;

    uint8_t data[4];

    /* Column Address Set (0x2A) */
    data[0] = (uint8_t)(x1 >> 8);
    data[1] = (uint8_t)(x1 & 0xFF);
    data[2] = (uint8_t)(x2 >> 8);
    data[3] = (uint8_t)(x2 & 0xFF);
    lcd_write_cmd(ST7789_CASET);
    lcd_write_bytes(data, 4);

    /* Row Address Set (0x2B) */
    data[0] = (uint8_t)(y1 >> 8);
    data[1] = (uint8_t)(y1 & 0xFF);
    data[2] = (uint8_t)(y2 >> 8);
    data[3] = (uint8_t)(y2 & 0xFF);
    lcd_write_cmd(ST7789_RASET);
    lcd_write_bytes(data, 4);

    /* Write to RAM (0x2C) */
    lcd_write_cmd(ST7789_RAMWR);
    bflb_gpio_set(gpio_dev, LCD_SPI_DC_PIN);
}

/* Initialize SPI controller and LCD panel */
void lcd_spi_init(void)
{
    printf("[LCD] Fetching device handles...\r\n");
    gpio_dev = bflb_device_get_by_name("gpio");
    spi_dev  = bflb_device_get_by_name("spi1");

    if (!spi_dev) {
        printf("[LCD ERROR] spi1 device handle is NULL!\r\n");
        return;
    }
    printf("[LCD] spi1 handle acquired (base: 0x%08lx). Setting up pins...\r\n", (unsigned long)spi_dev->reg_base);

    /* Initialize control GPIO pins */
    bflb_gpio_init(gpio_dev, LCD_SPI_DC_PIN, GPIO_OUTPUT | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);
    bflb_gpio_init(gpio_dev, LCD_SPI_RESET_PIN, GPIO_OUTPUT | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);
    bflb_gpio_init(gpio_dev, LCD_SPI_BACKLIGHT_PIN, GPIO_OUTPUT | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);

    /* Configure SPI1 CS, MOSI, and SCLK Alternate Function Pinmux */
    bflb_gpio_init(gpio_dev, LCD_SPI_CS_PIN, GPIO_FUNC_SPI1 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_2);
    bflb_gpio_init(gpio_dev, LCD_SPI_MOSI_PIN, GPIO_FUNC_SPI1 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_2);
    bflb_gpio_init(gpio_dev, LCD_SPI_SCLK_PIN, GPIO_FUNC_SPI1 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_2);

    bflb_gpio_set(gpio_dev, LCD_SPI_DC_PIN);
    bflb_gpio_set(gpio_dev, LCD_SPI_BACKLIGHT_PIN);

    /* Initialize SPI1 Peripheral at 40MHz for fast FPS */
    struct bflb_spi_config_s spi_cfg = {
        .freq = 40 * 1000 * 1000,
        .role = SPI_ROLE_MASTER,
        .mode = SPI_MODE0,
        .data_width = SPI_DATA_WIDTH_8BIT,
        .bit_order = SPI_BIT_MSB,
        .byte_order = SPI_BYTE_MSB,
        .tx_fifo_threshold = 0,
        .rx_fifo_threshold = 0,
    };
    bflb_spi_init(spi_dev, &spi_cfg);
    printf("[LCD] SPI1 peripheral initialized at 40 MHz.\r\n");

    /* Hardware Reset LCD */
    bflb_gpio_reset(gpio_dev, LCD_SPI_RESET_PIN);
    bflb_mtimer_delay_ms(20);
    bflb_gpio_set(gpio_dev, LCD_SPI_RESET_PIN);
    bflb_mtimer_delay_ms(20);

    /* Official ST7789V SPI Init Sequence */
    lcd_write_cmd(ST7789_SWRESET);
    bflb_mtimer_delay_ms(20);

    lcd_write_cmd(ST7789_SLPOUT);
    bflb_mtimer_delay_ms(20);

    /* Set Color Mode to 16-bit 65K Colors (RGB565) */
    lcd_write_cmd(ST7789_COLMOD);
    lcd_write_data8(0x55);
    bflb_mtimer_delay_ms(10);

    /* Inversion ON for IPS panel */
    lcd_write_cmd(ST7789_INVON);
    bflb_mtimer_delay_ms(20);

    /* Display ON */
    lcd_write_cmd(ST7789_DISPON);
    bflb_mtimer_delay_ms(20);

    /* Memory Access Control: 280x240 orientation (Landscape) */
    lcd_write_cmd(ST7789_MADCTL);
    lcd_write_data8(0x60);
    bflb_mtimer_delay_ms(10);

    /* Frame Rate Control */
    lcd_write_cmd(0xC6);
    lcd_write_data8(0x00);

    /* Clear initial display */
    lcd_clear(COLOR_BLACK);
}

/* Clear full screen with color */
void lcd_clear(uint16_t color)
{
    lcd_set_window(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
    
    for (int i = 0; i < SCREEN_WIDTH; i++) {
        line_buffer[i] = (color >> 8) | (color << 8); /* Big-endian for SPI */
    }

    bflb_gpio_set(gpio_dev, LCD_SPI_DC_PIN);

    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        bflb_spi_poll_exchange(spi_dev, line_buffer, NULL, SCREEN_WIDTH * 2);
    }
}

/* Fast block rectangle fill */
void lcd_draw_rect_fill(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    if (x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT || (x + w) <= 0 || (y + h) <= 0 || w <= 0 || h <= 0) {
        return;
    }

    int16_t x1 = (x < 0) ? 0 : x;
    int16_t y1 = (y < 0) ? 0 : y;
    int16_t x2 = (x + w > SCREEN_WIDTH) ? SCREEN_WIDTH - 1 : (x + w - 1);
    int16_t y2 = (y + h > SCREEN_HEIGHT) ? SCREEN_HEIGHT - 1 : (y + h - 1);
    int16_t draw_w = x2 - x1 + 1;

    lcd_set_window(x1, y1, x2, y2);

    uint16_t spi_color = (color >> 8) | (color << 8);
    for (int i = 0; i < draw_w; i++) {
        line_buffer[i] = spi_color;
    }

    bflb_gpio_set(gpio_dev, LCD_SPI_DC_PIN);

    for (int row = y1; row <= y2; row++) {
        bflb_spi_poll_exchange(spi_dev, line_buffer, NULL, draw_w * 2);
    }
}


/* Draw rectangle outline */
void lcd_draw_rect_outline(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    lcd_draw_rect_fill(x, y, w, 1, color);             /* Top */
    lcd_draw_rect_fill(x, y + h - 1, w, 1, color);     /* Bottom */
    lcd_draw_rect_fill(x, y, 1, h, color);             /* Left */
    lcd_draw_rect_fill(x + w - 1, y, 1, h, color);     /* Right */
}

/* Draw filled circle (Bresenham-based span filling) */
void lcd_draw_circle_fill(int16_t cx, int16_t cy, int16_t radius, uint16_t color)
{
    for (int16_t dy = -radius; dy <= radius; dy++) {
        int16_t r_sq = radius * radius;
        int16_t dy_sq = dy * dy;
        int16_t dx = 0;
        while ((dx * dx + dy_sq) <= r_sq) {
            dx++;
        }
        dx--;
        lcd_draw_rect_fill(cx - dx, cy + dy, 2 * dx + 1, 1, color);
    }
}

/* Flush raw pixel buffer to a bounding box */
void lcd_flush_rect(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *pixels)
{
    if (x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT || w <= 0 || h <= 0) return;
    int16_t x2 = x + w - 1;
    int16_t y2 = y + h - 1;
    lcd_set_window(x, y, x2, y2);

    bflb_gpio_set(gpio_dev, LCD_SPI_DC_PIN);
    bflb_spi_poll_exchange(spi_dev, (void *)pixels, NULL, w * h * 2);
}


/* Render single 5x7 character */
void lcd_draw_char(int16_t x, int16_t y, char c, uint16_t color, uint16_t bg_color)
{
    if (c < 32 || c > 126) c = ' ';
    const uint8_t *glyph = font5x7[c - 32];

    for (int8_t col = 0; col < 5; col++) {
        uint8_t line = glyph[col];
        for (int8_t row = 0; row < 7; row++) {
            if (line & (1 << row)) {
                lcd_draw_rect_fill(x + col, y + row, 1, 1, color);
            } else if (bg_color != color) {
                lcd_draw_rect_fill(x + col, y + row, 1, 1, bg_color);
            }
        }
    }
}

/* Render string */
void lcd_draw_string(int16_t x, int16_t y, const char *str, uint16_t color, uint16_t bg_color)
{
    int16_t cur_x = x;
    while (*str) {
        lcd_draw_char(cur_x, y, *str, color, bg_color);
        cur_x += 6; /* 5 pixels + 1 pixel letter space */
        str++;
    }
}

/* Render integer number */
void lcd_draw_num(int16_t x, int16_t y, int32_t num, uint16_t color, uint16_t bg_color)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%ld", (long)num);
    lcd_draw_string(x, y, buf, color, bg_color);
}

/* Control LCD backlight */
void lcd_set_backlight(uint8_t brightness)
{
    if (brightness > 0) {
        bflb_gpio_set(gpio_dev, LCD_SPI_BACKLIGHT_PIN);
    } else {
        bflb_gpio_reset(gpio_dev, LCD_SPI_BACKLIGHT_PIN);
    }
}
