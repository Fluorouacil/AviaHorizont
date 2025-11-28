#include "ST7735S.h"
#include "Font.h"

static void _spi_init(void) {
  // Настройка пинов SPI: MOSI (PB3), SCK (PB5) как выходы
  DDRB |= (1 << PB3) | (1 << PB5);

  // Включение SPI: Master, режим 0, частота F_CPU/4
  SPCR = (1 << SPE) | (1 << MSTR);
  SPSR = (1 << SPI2X); // Удвоение скорости (F_CPU/2)
}
static void _spi_write(uint8_t data) {
  SPDR = data;
  while (!(SPSR & (1 << SPIF)))
    ;
}

void _st7735s_write_command(uint8_t cmd) {
  DC_LOW();
  CS_LOW();
  _spi_write(cmd);
  CS_HIGH();
}
void _st7735s_write_data(uint8_t data) {
  DC_HIGH();
  CS_LOW();
  _spi_write(data);
  CS_HIGH();
}
void _st7735s_write_data_16(uint16_t data) {
  DC_HIGH();
  CS_LOW();
  _spi_write(data >> 8);   // Старший байт
  _spi_write(data & 0xFF); // Младший байт
  CS_HIGH();
}
void _st7735s_set_address_window(uint16_t x0, uint16_t y0, uint16_t x1,
                                uint16_t y1) {
  // Установка столбцов (CASET)
  _st7735s_write_command(0x2A);
  _st7735s_write_data(x0 >> 8);
  _st7735s_write_data(x0 & 0xFF);
  _st7735s_write_data(x1 >> 8);
  _st7735s_write_data(x1 & 0xFF);

  // Установка строк (RASET)
  _st7735s_write_command(0x2B);
  _st7735s_write_data(y0 >> 8);
  _st7735s_write_data(y0 & 0xFF);
  _st7735s_write_data(y1 >> 8);
  _st7735s_write_data(y1 & 0xFF);

  // Команда записи в память (RAMWR)
  _st7735s_write_command(0x2C);
}

void st7735s_init(void) {
  // Настройка пинов управления
  ST7735S_DDR |= (1 << DC_PIN) | (1 << RESET_PIN);
  #ifdef ST7735S_USE_CS
      ST7735S_DDR |= (1 << CS_PIN);
  #endif

  // Инициализация SPI
  _spi_init();

  // Процедура сброса дисплея
  RESET_HIGH();
  _delay_ms(10);
  RESET_LOW();
  _delay_ms(10);
  RESET_HIGH();
  _delay_ms(150);

  // Последовательность инициализации ST7735S
  _st7735s_write_command(0x01); // SWRESET: программный сброс
  _delay_ms(150);

  _st7735s_write_command(0x11); // SLPOUT: выход из спящего режима
  _delay_ms(255);

  _st7735s_write_command(0x3A); // COLMOD: установка формата цвета
  _st7735s_write_data(0x05);    // 16 бит на пиксель (RGB565)

  // ВАЖНО: Установка горизонтальной ориентации
  _st7735s_write_command(0x36);          // MADCTL: управление ориентацией
  _st7735s_write_data(MADCTL_LANDSCAPE); // Горизонтальная ориентация

  // Дополнительные настройки для лучшего отображения
  _st7735s_write_command(0xB2); // PORCTRL: настройка porch
  _st7735s_write_data(0x0C);
  _st7735s_write_data(0x0C);
  _st7735s_write_data(0x00);
  _st7735s_write_data(0x33);
  _st7735s_write_data(0x33);

  _st7735s_write_command(0xB7); // GCTRL: настройка gate control
  _st7735s_write_data(0x35);

  _st7735s_write_command(0xBB); // VCOMS: настройка VCOM
  _st7735s_write_data(0x2B);

  _st7735s_write_command(0xC0); // LCMCTRL: настройка LCM
  _st7735s_write_data(0x2C);

  _st7735s_write_command(0xC2); // VDVVRHEN: настройка VDV и VRH
  _st7735s_write_data(0x01);
  _st7735s_write_data(0xFF);

  _st7735s_write_command(0xC3); // VRHS: настройка VRH
  _st7735s_write_data(0x11);

  _st7735s_write_command(0xC4); // VDVS: настройка VDV
  _st7735s_write_data(0x20);

  _st7735s_write_command(0xC6); // FRCTRL2: настройка частоты
  _st7735s_write_data(0x0F);

  _st7735s_write_command(0xD0); // PWCTRL1: настройка питания
  _st7735s_write_data(0xA4);
  _st7735s_write_data(0xA1);

  // Включение нормального режима и дисплея
  _st7735s_write_command(0x13); // NORON: нормальный режим
  _delay_ms(10);

  _st7735s_write_command(0x29); // DISPON: включение дисплея
  _delay_ms(100);
}

void st7735s_fill_screen(uint16_t color) {
  // Установка окна на весь экран
  _st7735s_set_address_window(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);

  // Быстрая заливка через непрерывную передачу
  DC_HIGH();
  CS_LOW();

  uint32_t total_pixels = (uint32_t)DISPLAY_WIDTH * DISPLAY_HEIGHT;

  // Оптимизированная заливка экрана
  for (uint32_t i = 0; i < total_pixels; i++) {
    _spi_write(color >> 8);   // Старший байт цвета
    _spi_write(color & 0xFF); // Младший байт цвета
  }

  CS_HIGH();
}

void st7735s_draw_hline(uint16_t x, uint16_t y, uint16_t length,
                       uint16_t color) {
  // Проверка границ
  if (y >= DISPLAY_HEIGHT)
    return;
  if (x >= DISPLAY_WIDTH)
    return;

  uint16_t x_end = x + length - 1;
  if (x_end >= DISPLAY_WIDTH)
    x_end = DISPLAY_WIDTH - 1;

  _st7735s_set_address_window(x, y, x_end, y);

  DC_HIGH();
  CS_LOW();

  for (uint16_t i = x; i <= x_end; i++) {
    _spi_write(color >> 8);
    _spi_write(color & 0xFF);
  }

  CS_HIGH();
}

void st7735s_draw_vline(uint16_t x, uint16_t y, uint16_t length,
                       uint16_t color) {
  // Проверка границ
  if (x >= DISPLAY_WIDTH)
    return;
  if (y >= DISPLAY_HEIGHT)
    return;

  uint16_t y_end = y + length - 1;
  if (y_end >= DISPLAY_HEIGHT)
    y_end = DISPLAY_HEIGHT - 1;

  _st7735s_set_address_window(x, y, x, y_end);

  DC_HIGH();
  CS_LOW();

  for (uint16_t i = y; i <= y_end; i++) {
    _spi_write(color >> 8);
    _spi_write(color & 0xFF);
  }

  CS_HIGH();
}

void st7735s_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                      uint16_t color) {
  // Оптимизация для горизонтальных и вертикальных линий
  if (y0 == y1) {
    if (x0 < x1)
      st7735s_draw_hline(x0, y0, x1 - x0 + 1, color);
    else
      st7735s_draw_hline(x1, y0, x0 - x1 + 1, color);
    return;
  }

  if (x0 == x1) {
    if (y0 < y1)
      st7735s_draw_vline(x0, y0, y1 - y0 + 1, color);
    else
      st7735s_draw_vline(x0, y1, y0 - y1 + 1, color);
    return;
  }

  // Алгоритм Брезенхэма для произвольных линий
  int16_t dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
  int16_t dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
  int16_t sx = (x0 < x1) ? 1 : -1;
  int16_t sy = (y0 < y1) ? 1 : -1;
  int16_t err = dx - dy;

  while (1) {
    // Рисование пикселя (с проверкой границ)
    if (x0 < DISPLAY_WIDTH && y0 < DISPLAY_HEIGHT) {
      _st7735s_set_address_window(x0, y0, x0, y0);
      _st7735s_write_data_16(color);
    }

    if (x0 == x1 && y0 == y1)
      break;

    int16_t e2 = 2 * err;
    if (e2 > -dy) {
      err -= dy;
      x0 += sx;
    }
    if (e2 < dx) {
      err += dx;
      y0 += sy;
    }
  }
}

void st7735s_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT) return;
    if (x + w > DISPLAY_WIDTH) w = DISPLAY_WIDTH - x;
    if (y + h > DISPLAY_HEIGHT) h = DISPLAY_HEIGHT - y;
    if (w == 0 || h == 0) return;

    _st7735s_set_address_window(x, y, x + w - 1, y + h - 1);

    DC_HIGH();
    CS_LOW();

    uint32_t pixels = (uint32_t)w * h;
    for (uint32_t i = 0; i < pixels; i++) {
        _spi_write(color >> 8);
        _spi_write(color & 0xFF);
    }

    CS_HIGH();
}

void st7735s_fill_rect_mirror_x(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    int16_t x_mirrored = DISPLAY_WIDTH - 1 - (x + w - 1);
    if (x_mirrored < 0) x_mirrored = 0;
    st7735s_fill_rect(x_mirrored, y, w, h, color);
}

void st7735s_draw_pixel(uint16_t x, uint16_t y, uint16_t color) {
    x = DISPLAY_WIDTH - 1 - x;
    if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT || x < 0 || y < 0) return;
    st7735s_fill_rect(x, y, 1, 1, color);
}

void st7735s_draw_digit(int16_t x, int16_t y, char c, uint16_t color, uint8_t size) {
    uint8_t idx;
    switch (c) {
        case '-': idx = 0; break;
        case '+': idx = 1; break;
        case '.': idx = 2; break;
        case '°':
        case 176: idx = 3; break;
        case '0': idx = 4; break;
        case '1': idx = 5; break;
        case '2': idx = 6; break;
        case '3': idx = 7; break;
        case '4': idx = 8; break;
        case '5': idx = 9; break;
        case '6': idx = 10; break;
        case '7': idx = 11; break;
        case '8': idx = 12; break;
        case '9': idx = 13; break;
        default: return;
    }

    for (uint8_t col = 0; col < 5; col++) {
        uint8_t bits = pgm_read_byte(&tiny_font[idx * 5 + col]);
        for (uint8_t row = 0; row < 7; row++) {
            if (bits & (1 << (6 - row))) {
                if (size == 1) {
                    st7735s_draw_pixel(x + row, y + (4 - col), color);
                } else if (size == 2) {
                    st7735s_fill_rect(x + row * 2, y + (4 - col) * 2, 2, 2, color);
                }
            }
        }
    }
}

void st7735s_draw_number_string(int16_t x, int16_t y, const char *str, uint16_t color, uint8_t size) {
    uint8_t step = (size == 1) ? 7 : 14;  // 7 = 6 (макс row) + 1 отступ
    while (*str) {
        st7735s_draw_digit(x, y, *str, color, size);
        x += step;
        str++;
    }
}

void st7735s_draw_angle(int16_t x, int16_t y, int16_t deg, uint16_t color, uint8_t size) {
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

    st7735s_draw_number_string(x, y, buf, color, size);
}