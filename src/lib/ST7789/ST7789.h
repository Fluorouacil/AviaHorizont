#ifndef ST7789_H
#define ST7789_H

#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>
#include <stdbool.h>
#include <stdlib.h>

// === ЦВЕТА (RGB565) ===
#define RGB565(r, g, b) ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3))

#define COLOR_BLACK   RGB565(  0,   0,   0)
#define COLOR_WHITE   RGB565(255, 255, 255)
#define COLOR_RED     RGB565(255,   0,   0)
#define COLOR_GREEN   RGB565(  0, 255,   0)
#define COLOR_BLUE    RGB565(  0,   0, 255)
#define COLOR_YELLOW  RGB565(255, 255,   0)
#define COLOR_CYAN    RGB565(  0, 255, 255)
#define COLOR_MAGENTA RGB565(255,   0, 255)

// === РАЗРЕШЕНИЕ ===
#ifndef DISPLAY_WIDTH
    #define DISPLAY_WIDTH  240
#endif
#ifndef DISPLAY_HEIGHT
    #define DISPLAY_HEIGHT 240
#endif

// === СМЕЩЕНИЯ (для cheap 240×240 панелей: часто 40, 53) ===
#ifndef COLSTART
    #define COLSTART 0   // обычно 0 или 40
#endif
#ifndef ROWSTART
    #define ROWSTART 0   // обычно 0 или 53
#endif

// === ПИНЫ (настрой под свою плату) ===
#define ST7789_PORT PORTB
#define ST7789_DDR  DDRB

#define DC_PIN    PB0   // D8
#define RESET_PIN PB1   // D9
// #define CS_PIN PB2   // D10 ← раскомментируй ТОЛЬКО если есть CS

// #define ST7789_USE_CS    // ← раскомментируй, если есть пин CS
#define ST7789_NO_CS        // ← раскомментируй, если CS отсутствует (всегда активен)

// --- Проверка конфигурации ---
#ifdef ST7789_USE_CS
    #ifndef CS_PIN
        #error "CS_PIN must be defined when ST7789_USE_CS is enabled"
    #endif
    #define _CS_HIGH() (ST7789_PORT |= (1 << CS_PIN))
    #define _CS_LOW()  (ST7789_PORT &= ~(1 << CS_PIN))
#elif defined(ST7789_NO_CS)
    #define _CS_HIGH() ((void)0)
    #define _CS_LOW()  ((void)0)
#else
    #error "Define either ST7789_USE_CS or ST7789_NO_CS"
#endif

// --- Макросы управления пинами ---
#define DC_HIGH()    (ST7789_PORT |= (1 << DC_PIN))
#define DC_LOW()     (ST7789_PORT &= ~(1 << DC_PIN))
#define RESET_HIGH() (ST7789_PORT |= (1 << RESET_PIN))
#define RESET_LOW()  (ST7789_PORT &= ~(1 << RESET_PIN))

// Совместимость (если где-то используется старое имя)
#define CS_HIGH() _CS_HIGH()
#define CS_LOW()  _CS_LOW()

// === MADCTL (Memory Data Access Control) ===
#define MADCTL_MY  0x80  // Row address order (0: top→bottom, 1: bottom→top)
#define MADCTL_MX  0x40  // Column address order (0: left→right, 1: right→left)
#define MADCTL_MV  0x20  // Row/Column exchange (0: normal, 1: swapped)
#define MADCTL_ML  0x10  // Vertical refresh order (не используется в ST7789)
#define MADCTL_RGB 0x00  // RGB order
#define MADCTL_BGR 0x08  // BGR order (часто требуется)
#define MADCTL_MH  0x04  // Horizontal refresh order (не используется)

// === ОРИЕНТАЦИИ (адаптируй под свою панель) ===
#define MADCTL_LANDSCAPE      (MADCTL_MX | MADCTL_MV | MADCTL_BGR)  // 0°   — ширина = 240
#define MADCTL_PORTRAIT       (MADCTL_MY | MADCTL_MV | MADCTL_BGR)  // 90°  — высота = 240
#define MADCTL_LANDSCAPE_REV  (MADCTL_MY | MADCTL_BGR)              // 180°
#define MADCTL_PORTRAIT_REV   (MADCTL_MX | MADCTL_BGR)              // 270°

// === ПРОТОТИПЫ ФУНКЦИЙ ===
void st7789_init(void);
void st7789_fill_screen(uint16_t color);
void st7789_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void st7789_draw_hline(uint16_t x, uint16_t y, uint16_t w, uint16_t color);
void st7789_draw_vline(uint16_t x, uint16_t y, uint16_t h, uint16_t color);
void st7789_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
void st7789_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
void st7789_fill_rect_mirror_x(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void st7789_draw_digit(int16_t x, int16_t y, char c, uint16_t color, uint8_t size);
void st7789_draw_number_string(int16_t x, int16_t y, const char *str, uint16_t color, uint8_t size);
void st7789_draw_angle(int16_t x, int16_t y, int16_t deg, uint16_t color, uint8_t size);

// === ВНУТРЕННЯЯ ФУНКЦИЯ (не обязана быть в .h, но иногда удобно) ===
void _st7789_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

#endif // ST7789_H