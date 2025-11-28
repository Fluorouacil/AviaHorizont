#ifndef ST7735S_H
#define ST7735S_H

#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>
#include <stdbool.h>
#include <stdlib.h>

// === ЦВЕТА ===
#define RGB565(r, g, b) ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3))

#define COLOR_BLACK RGB565(0,   0,   0)
#define COLOR_WHITE RGB565(255, 255, 255)

// === РАЗРЕШЕНИЕ ===
#ifndef DISPLAY_WIDTH
    #define DISPLAY_WIDTH  160
#endif
#ifndef DISPLAY_HEIGHT
    #define DISPLAY_HEIGHT 128
#endif

// === ПИНЫ (настрой под свою плату) ===
#define ST7735S_PORT PORTB
#define ST7735S_DDR  DDRB

#define DC_PIN    PB0   // D8
#define RESET_PIN PB1   // D9
#define CS_PIN PB2   // D10 ← раскомментируй ТОЛЬКО если есть CS

#define ST7735S_USE_CS    // ← раскомментируй, если есть пин CS
// #define ST7735S_NO_CS        // ← раскомментируй, если CS отсутствует (всегда активен)

// --- Проверка конфигурации ---
#ifdef ST7735S_USE_CS
    #ifndef CS_PIN
        #error "CS_PIN must be defined when ST7735S_USE_CS is enabled"
    #endif
    #define _CS_HIGH() (ST7735S_PORT |= (1 << CS_PIN))
    #define _CS_LOW()  (ST7735S_PORT &= ~(1 << CS_PIN))
#elif defined(ST7735S_NO_CS)
    #define _CS_HIGH() ((void)0)
    #define _CS_LOW()  ((void)0)
#else
    #error "Define either ST7735S_USE_CS or ST7735S_NO_CS"
#endif

// --- Макросы (всегда доступны) ---
#define DC_HIGH()   (ST7735S_PORT |= (1 << DC_PIN))
#define DC_LOW()    (ST7735S_PORT &= ~(1 << DC_PIN))
#define RESET_HIGH()(ST7735S_PORT |= (1 << RESET_PIN))
#define RESET_LOW() (ST7735S_PORT &= ~(1 << RESET_PIN))

// Совместимость: оставим старые имена для твоего .c-файла
#define CS_HIGH() _CS_HIGH()
#define CS_LOW()  _CS_LOW()

// === MADCTL ===
#define MADCTL_MY  0x80
#define MADCTL_MX  0x40
#define MADCTL_MV  0x20
#define MADCTL_ML  0x10
#define MADCTL_RGB 0x00
#define MADCTL_BGR 0x08
#define MADCTL_MH  0x04
#define MADCTL_LANDSCAPE (MADCTL_MX | MADCTL_MV)

// === ПРОТОТИПЫ ===
void st7735s_init(void);
void st7735s_fill_screen(uint16_t color);
void st7735s_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
void st7735s_draw_hline(uint16_t x, uint16_t y, uint16_t length, uint16_t color);
void st7735s_draw_vline(uint16_t x, uint16_t y, uint16_t length, uint16_t color);
void st7735s_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void st7735s_fill_rect_mirror_x(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void st7735s_draw_angle(int16_t x, int16_t y, int16_t deg, uint16_t color, uint8_t size);
void st7735s_draw_number_string(int16_t x, int16_t y, const char *str, uint16_t color, uint8_t size);
void st7735s_draw_digit(int16_t x, int16_t y, char c, uint16_t color, uint8_t size);

#endif // ST7735S_H