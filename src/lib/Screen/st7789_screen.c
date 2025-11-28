// ./lib/screen/st7789_screen.c
#include "screen.h"
#include "../ST7789/ST7789.h"

// Конвертер цвета → RGB565 (предполагаем, что ваша ST7789.h использует RGB565)
static inline uint16_t rgb565(const Color* c) {
    return ((c->red & 0xF8) << 8) | ((c->green & 0xFC) << 3) | (c->blue >> 3);
}

// Реализации
static void fill_rect_impl(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const Color* color) {
    st7789_fill_rect(x, y, w, h, rgb565(color));
}

static void draw_line_impl(const Point2D* p1, const Point2D* p2, const Color* color) {
    st7789_draw_line(p1->x, p1->y, p2->x, p2->y, rgb565(color));
}

static void draw_hline_impl(uint16_t x, uint16_t y, uint16_t w, const Color* color) {
    st7789_draw_hline(x, y, w, rgb565(color));
}

static void draw_vline_impl(uint16_t x, uint16_t y, uint16_t h, const Color* color) {
    st7789_draw_vline(x, y, h, rgb565(color));
}

static void draw_string_impl(uint16_t x, uint16_t y, const char* str, const Color* color, uint8_t scale) {
    st7789_draw_number_string(x, y, (char*)str, rgb565(color), scale);
}

static void clear_impl(const Color* color) {
    st7789_fill_screen(rgb565(color));
}

// Глобальный объект экрана (готов к использованию)
const Screen ST7789_SCREEN = {
    .width = DISPLAY_WIDTH,
    .height = DISPLAY_HEIGHT,
    .fill_rect = fill_rect_impl,
    .draw_line = draw_line_impl,
    .draw_hline = draw_hline_impl,
    .draw_vline = draw_vline_impl,
    .draw_string = draw_string_impl,
    .clear = clear_impl
};