// ./lib/screen/screen.h
#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>
#include <stdbool.h>
#include <avr/pgmspace.h>  // ← добавлено

typedef struct {
    uint8_t red, green, blue;
} Color;

typedef struct {
    int16_t x, y;
} Point2D;

typedef struct {
    uint16_t width;
    uint16_t height;

    void (*fill_rect)(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const Color* color);
    void (*draw_line)(const Point2D* p1, const Point2D* p2, const Color* color);
    void (*draw_hline)(uint16_t x, uint16_t y, uint16_t w, const Color* color);
    void (*draw_vline)(uint16_t x, uint16_t y, uint16_t h, const Color* color);
    void (*draw_string)(uint16_t x, uint16_t y, const char* str, const Color* color, uint8_t scale);
    void (*clear)(const Color* color);
} Screen;

static inline Point2D make_point(int16_t x, int16_t y) {
    Point2D p = {x, y};
    return p;
}

static inline Color make_color(uint8_t r, uint8_t g, uint8_t b) {
    Color c = {r, g, b};
    return c;
}

// Макрос для безопасного вызова функций из PROGMEM
#define SCREEN_CALL(screen, func, ...) \
    ({ \
        typeof(screen) _s = (screen); \
        void (*_f)(void) = (void(*)(void))pgm_read_ptr(&(_s)->func); \
        ((typeof(&(_s)->func))_f)(__VA_ARGS__); \
    })

#endif // SCREEN_H