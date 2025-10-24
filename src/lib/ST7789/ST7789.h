#ifndef ST7789_H
#define ST7789_H

#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>

// --- Определения общих типов ---
typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} Color;

typedef struct {
    int16_t coordinates[2]; // [x, y]
} ScreenPoint;

typedef struct {
    int width;
    int height;
    void (*draw_line)(const ScreenPoint *p1, const ScreenPoint *p2, const Color *color);
} Screen;


#define RGB565(r, g, b)                                                        \
  ((((255 - r) & 0xF8) << 8) | (((255 - g) & 0xFC) << 3) | ((255 - b) >> 3))

#define COLOR_BLACK RGB565(0, 0, 0)
#define COLOR_WHITE RGB565(255, 255, 255)

#define DISPLAY_WIDTH 160
#define DISPLAY_HEIGHT 128
#define ST7789_PORT PORTB
#define ST7789_DDR DDRB
#define DC_PIN PB0    // Data/Command
#define RESET_PIN PB1 // Reset
#define CS_PIN PB2    // Chip Select

#define MADCTL_MY 0x80  // Page Address Order (обратный по Y)
#define MADCTL_MX 0x40  // Column Address Order (обратный по X)
#define MADCTL_MV 0x20  // Page/Column Order (перестановка X/Y)
#define MADCTL_ML 0x10  // Line Address Order (обратный порядок строк)
#define MADCTL_RGB 0x00 // RGB порядок цветов
#define MADCTL_BGR 0x08 // BGR порядок цветов
#define MADCTL_MH 0x04  // Display Data Latch Order
#define MADCTL_LANDSCAPE (MADCTL_MX)

void st7789_init(void);

void st7789_fill_screen(uint16_t color);
void st7789_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                      uint16_t color);

void st7789_draw_hline(uint16_t x, uint16_t y, uint16_t length, uint16_t color);
void st7789_draw_vline(uint16_t x, uint16_t y, uint16_t length, uint16_t color);

// void _st7789_write_command(uint8_t cmd);
// void _st7789_write_data(uint8_t data);
// void _st7789_write_data_16(uint16_t data);
// void _st7789_set_address_window(uint16_t x0, uint16_t y0, uint16_t x1,
//                                 uint16_t y1);

#endif // ST7789_H