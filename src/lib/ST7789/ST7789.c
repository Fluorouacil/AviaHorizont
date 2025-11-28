#include "ST7789.h"
#include "Font.h"

// --- Внутренние функции ---
static void _spi_init(void) {
    DDRB |= (1 << PB3) | (1 << PB5); // MOSI, SCK → выход
    SPCR = (1 << SPE) | (1 << MSTR); // SPI Master, Mode 0
    SPSR = (1 << SPI2X);             // Удвоение → F_CPU/2 (8 МГц при 16 МГц)
}

static void _spi_write(uint8_t data) {
    SPDR = data;
    while (!(SPSR & (1 << SPIF))) ;
}

// 🔹 КОМАНДА: DC=0, передаём 1 байт
static void _st7789_write_cmd(uint8_t cmd) {
    DC_LOW();
    _CS_LOW();
    _spi_write(cmd);
    _CS_HIGH();
}

// 🔹 ДАННЫЕ: DC=1, передаём 1 байт
static void _st7789_write_data8(uint8_t data) {
    DC_HIGH();
    _CS_LOW();
    _spi_write(data);
    _CS_HIGH();
}

// 🔹 ДАННЫЕ: 16-бит (2 байта)
static void _st7789_write_data16(uint16_t data) {
    DC_HIGH();
    _CS_LOW();
    _spi_write(data >> 8);
    _spi_write(data & 0xFF);
    _CS_HIGH();
}

// 🔹 БЛОК ДАННЫХ (быстро, без переключения CS/DC внутри)
static void _st7789_write_data_block(const uint8_t *data, uint16_t len) {
    DC_HIGH();
    _CS_LOW();
    for (uint16_t i = 0; i < len; i++) {
        _spi_write(data[i]);
    }
    _CS_HIGH();
}

// 🔹 УСТАНОВКА АДРЕСНОГО ОКНА (с учётом COLSTART/ROWSTART)
void _st7789_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    // Коррекция на физическое смещение дисплея
    x0 += COLSTART; x1 += COLSTART;
    y0 += ROWSTART; y1 += ROWSTART;

    // CASET: столбцы
    _st7789_write_cmd(0x2A);
    _st7789_write_data8(x0 >> 8);
    _st7789_write_data8(x0 & 0xFF);
    _st7789_write_data8(x1 >> 8);
    _st7789_write_data8(x1 & 0xFF);

    // RASET: строки
    _st7789_write_cmd(0x2B);
    _st7789_write_data8(y0 >> 8);
    _st7789_write_data8(y0 & 0xFF);
    _st7789_write_data8(y1 >> 8);
    _st7789_write_data8(y1 & 0xFF);

    // RAMWR: готовы к записи пикселей
    _st7789_write_cmd(0x2C);
}

// ✅ ИНИЦИАЛИЗАЦИЯ ST7789 (рабочая)
void st7789_init(void) {
    // Настройка GPIO
    ST7789_DDR |= (1 << DC_PIN) | (1 << RESET_PIN);
#ifdef ST7789_USE_CS
    ST7789_DDR |= (1 << CS_PIN);
    _CS_HIGH();
#else
    // CS всегда LOW (если пин не подключён)
    // но лучше подключить CS к GND, чтобы не мешал SPI
#endif

    _spi_init();

    // Hardware reset
    RESET_HIGH(); _delay_ms(5);
    RESET_LOW();  _delay_ms(20);
    RESET_HIGH(); _delay_ms(150);

    // ⚙️ Инициализация (последовательность от Adafruit + Bodmer)
    _st7789_write_cmd(0x11); _delay_ms(10); // SLPOUT
    _st7789_write_cmd(0x36); _st7789_write_data8(MADCTL_LANDSCAPE); // MADCTL

    _st7789_write_cmd(0x3A); _st7789_write_data8(0x05); // COLMOD: 16-bit (RGB565)

    // Пороги porch (стандартные)
    _st7789_write_cmd(0xB2);
    _st7789_write_data8(0x0C); _st7789_write_data8(0x0C);
    _st7789_write_data8(0x00); _st7789_write_data8(0x33);
    _st7789_write_data8(0x33);

    // Gate control
    _st7789_write_cmd(0xB7);
    _st7789_write_data8(0x35);

    // VCOM
    _st7789_write_cmd(0xBB);
    _st7789_write_data8(0x19); // 0x19–0x2B (часто 0x19 для 3.3 В)

    // LCM Control — ОСТОРОЖНО: у ST7789 это 0xC0, но НЕ у всех!
    // Некоторые cheap-панели его не принимают → закомментируй, если глючит
    /*
    _st7789_write_cmd(0xC0);
    _st7789_write_data8(0x2C);
    */

    // VDV и VRH
    _st7789_write_cmd(0xC2);
    _st7789_write_data8(0x01);

    // Gamma (опционально, но рекомендовано)
    _st7789_write_cmd(0xE0);
    const uint8_t gamma_pos[] = {0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54,
                                  0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23};
    _st7789_write_data_block(gamma_pos, sizeof(gamma_pos));

    _st7789_write_cmd(0xE1);
    const uint8_t gamma_neg[] = {0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44,
                                  0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23};
    _st7789_write_data_block(gamma_neg, sizeof(gamma_neg));

    // Включение дисплея
    _st7789_write_cmd(0x29); // DISPON
    _delay_ms(100);
}

// ✅ Заливка экрана (оптимизировано: CS внизу на всё окно)
void st7789_fill_screen(uint16_t color) {
    _st7789_set_window(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);
    DC_HIGH();
    _CS_LOW();

    uint32_t pixels = (uint32_t)DISPLAY_WIDTH * DISPLAY_HEIGHT;
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;

    for (uint32_t i = 0; i < pixels; i++) {
        _spi_write(hi);
        _spi_write(lo);
    }
    _CS_HIGH();
}

// ✅ Горизонтальная линия
void st7789_draw_hline(uint16_t x, uint16_t y, uint16_t w, uint16_t color) {
    if (y >= DISPLAY_HEIGHT || x >= DISPLAY_WIDTH || w == 0) return;
    if (x + w > DISPLAY_WIDTH) w = DISPLAY_WIDTH - x;

    _st7789_set_window(x, y, x + w - 1, y);
    DC_HIGH();
    _CS_LOW();

    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    for (uint16_t i = 0; i < w; i++) {
        _spi_write(hi);
        _spi_write(lo);
    }
    _CS_HIGH();
}

// ✅ Вертикальная линия
void st7789_draw_vline(uint16_t x, uint16_t y, uint16_t h, uint16_t color) {
    if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT || h == 0) return;
    if (y + h > DISPLAY_HEIGHT) h = DISPLAY_HEIGHT - y;

    _st7789_set_window(x, y, x, y + h - 1);
    DC_HIGH();
    _CS_LOW();

    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    for (uint16_t i = 0; i < h; i++) {
        _spi_write(hi);
        _spi_write(lo);
    }
    _CS_HIGH();
}

// ✅ Прямоугольник
void st7789_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT || w == 0 || h == 0) return;
    if (x + w > DISPLAY_WIDTH) w = DISPLAY_WIDTH - x;
    if (y + h > DISPLAY_HEIGHT) h = DISPLAY_HEIGHT - y;

    _st7789_set_window(x, y, x + w - 1, y + h - 1);
    DC_HIGH();
    _CS_LOW();

    uint32_t pixels = (uint32_t)w * h;
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    for (uint32_t i = 0; i < pixels; i++) {
        _spi_write(hi);
        _spi_write(lo);
    }
    _CS_HIGH();
}

// ✅ Пиксель (без зеркалирования x!)
void st7789_draw_pixel(uint16_t x, uint16_t y, uint16_t color) {
    if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT) return;
    _st7789_set_window(x, y, x, y);
    _st7789_write_data16(color);
}

// ✅ Линия (Брезенхэм с оптимизацией под hline/vline)
void st7789_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color) {
    if (x0 == x1) { st7789_draw_vline(x0, y0, (y1 > y0) ? y1 - y0 + 1 : y0 - y1 + 1, color); return; }
    if (y0 == y1) { st7789_draw_hline(x0, y0, (x1 > x0) ? x1 - x0 + 1 : x0 - x1 + 1, color); return; }

    int16_t dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int16_t dy = y1 > y0 ? y1 - y0 : y0 - y1;
    int16_t sx = x0 < x1 ? 1 : -1;
    int16_t sy = y0 < y1 ? 1 : -1;
    int16_t err = dx - dy;

    while (1) {
        if (x0 < DISPLAY_WIDTH && y0 < DISPLAY_HEIGHT && x0 >= 0 && y0 >= 0)
            st7789_draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int16_t e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}

// ✅ Зеркальное отражение по X (если нужно)
void st7789_fill_rect_mirror_x(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    int16_t x_mirr = DISPLAY_WIDTH - 1 - (x + w - 1);
    if (x_mirr < 0) x_mirr = 0;
    st7789_fill_rect(x_mirr, y, w, h, color);
}

// ✅ Рисование цифры (исправлено: убрано x = width-1-x)
void st7789_draw_digit(int16_t x, int16_t y, char c, uint16_t color, uint8_t size) {
    uint8_t idx;
    switch (c) {
        case '-': idx = 0; break; case '+': idx = 1; break;
        case '.': idx = 2; break; case '°': case 176: idx = 3; break;
        case '0': idx = 4; break; case '1': idx = 5; break;
        case '2': idx = 6; break; case '3': idx = 7; break;
        case '4': idx = 8; break; case '5': idx = 9; break;
        case '6': idx = 10; break; case '7': idx = 11; break;
        case '8': idx = 12; break; case '9': idx = 13; break;
        default: return;
    }

    for (uint8_t col = 0; col < 5; col++) {
        uint8_t bits = pgm_read_byte(&tiny_font[idx * 5 + col]);
        for (uint8_t row = 0; row < 7; row++) {
            if (bits & (1 << (6 - row))) {
                if (size == 1) {
                    st7789_draw_pixel(x + row, y + (4 - col), color);
                } else if (size == 2) {
                    st7789_fill_rect(x + row * 2, y + (4 - col) * 2, 2, 2, color);
                }
            }
        }
    }
}

void st7789_draw_number_string(int16_t x, int16_t y, const char *str, uint16_t color, uint8_t size) {
    uint8_t step = (size == 1) ? 7 : 14;  // 7 = 6 (макс row) + 1 отступ
    while (*str) {
        st7789_draw_digit(x, y, *str, color, size);
        x += step;
        str++;
    }
}

void st7789_draw_angle(int16_t x, int16_t y, int16_t deg, uint16_t color, uint8_t size) {
    char buf[8];
    int len = 0;

    // Знак
    if (deg < 0) {
        buf[len++] = '-';
        deg = -deg;
    } else {
        buf[len++] = '+';
    }

    // Десятки и единицы (поддержка -99..+99)
    if (deg >= 10) {
        buf[len++] = '0' + (deg / 10);
    }
    buf[len++] = '0' + (deg % 10);
    buf[len++] = '°';  // символ градуса
    buf[len] = '\0';

    st7789_draw_number_string(x, y, buf, color, size);
}