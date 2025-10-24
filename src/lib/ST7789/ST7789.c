#include "ST7789.h"

#define DC_HIGH() (ST7789_PORT |= (1 << DC_PIN))
#define DC_LOW() (ST7789_PORT &= ~(1 << DC_PIN))
#define RESET_HIGH() (ST7789_PORT |= (1 << RESET_PIN))
#define RESET_LOW() (ST7789_PORT &= ~(1 << RESET_PIN))
#define CS_HIGH() (ST7789_PORT |= (1 << CS_PIN))
#define CS_LOW() (ST7789_PORT &= ~(1 << CS_PIN))

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

void _st7789_write_command(uint8_t cmd) {
  DC_LOW();
  CS_LOW();
  _spi_write(cmd);
  CS_HIGH();
}
void _st7789_write_data(uint8_t data) {
  DC_HIGH();
  CS_LOW();
  _spi_write(data);
  CS_HIGH();
}
void _st7789_write_data_16(uint16_t data) {
  DC_HIGH();
  CS_LOW();
  _spi_write(data >> 8);   // Старший байт
  _spi_write(data & 0xFF); // Младший байт
  CS_HIGH();
}
void _st7789_set_address_window(uint16_t x0, uint16_t y0, uint16_t x1,
                                uint16_t y1) {
  // Установка столбцов (CASET)
  _st7789_write_command(0x2A);
  _st7789_write_data(x0 >> 8);
  _st7789_write_data(x0 & 0xFF);
  _st7789_write_data(x1 >> 8);
  _st7789_write_data(x1 & 0xFF);

  // Установка строк (RASET)
  _st7789_write_command(0x2B);
  _st7789_write_data(y0 >> 8);
  _st7789_write_data(y0 & 0xFF);
  _st7789_write_data(y1 >> 8);
  _st7789_write_data(y1 & 0xFF);

  // Команда записи в память (RAMWR)
  _st7789_write_command(0x2C);
}

void st7789_init(void) {
  // Настройка пинов управления
  ST7789_DDR |= (1 << DC_PIN) | (1 << RESET_PIN) | (1 << CS_PIN);

  // Инициализация SPI
  _spi_init();

  // Процедура сброса дисплея
  RESET_HIGH();
  _delay_ms(10);
  RESET_LOW();
  _delay_ms(10);
  RESET_HIGH();
  _delay_ms(150);

  // Последовательность инициализации ST7789
  _st7789_write_command(0x01); // SWRESET: программный сброс
  _delay_ms(150);

  _st7789_write_command(0x11); // SLPOUT: выход из спящего режима
  _delay_ms(255);

  _st7789_write_command(0x3A); // COLMOD: установка формата цвета
  _st7789_write_data(0x05);    // 16 бит на пиксель (RGB565)

  // ВАЖНО: Установка горизонтальной ориентации
  _st7789_write_command(0x36);          // MADCTL: управление ориентацией
  _st7789_write_data(MADCTL_LANDSCAPE); // Горизонтальная ориентация

  // Дополнительные настройки для лучшего отображения
  _st7789_write_command(0xB2); // PORCTRL: настройка porch
  _st7789_write_data(0x0C);
  _st7789_write_data(0x0C);
  _st7789_write_data(0x00);
  _st7789_write_data(0x33);
  _st7789_write_data(0x33);

  _st7789_write_command(0xB7); // GCTRL: настройка gate control
  _st7789_write_data(0x35);

  _st7789_write_command(0xBB); // VCOMS: настройка VCOM
  _st7789_write_data(0x2B);

  _st7789_write_command(0xC0); // LCMCTRL: настройка LCM
  _st7789_write_data(0x2C);

  _st7789_write_command(0xC2); // VDVVRHEN: настройка VDV и VRH
  _st7789_write_data(0x01);
  _st7789_write_data(0xFF);

  _st7789_write_command(0xC3); // VRHS: настройка VRH
  _st7789_write_data(0x11);

  _st7789_write_command(0xC4); // VDVS: настройка VDV
  _st7789_write_data(0x20);

  _st7789_write_command(0xC6); // FRCTRL2: настройка частоты
  _st7789_write_data(0x0F);

  _st7789_write_command(0xD0); // PWCTRL1: настройка питания
  _st7789_write_data(0xA4);
  _st7789_write_data(0xA1);

  // Включение нормального режима и дисплея
  _st7789_write_command(0x21); // INVON: инверсия цветов (опционально)
  _st7789_write_command(0x13); // NORON: нормальный режим
  _delay_ms(10);

  _st7789_write_command(0x29); // DISPON: включение дисплея
  _delay_ms(100);
}

void st7789_fill_screen(uint16_t color) {
  // Установка окна на весь экран
  _st7789_set_address_window(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);

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

void st7789_draw_hline(uint16_t x, uint16_t y, uint16_t length,
                       uint16_t color) {
  // Проверка границ
  if (y >= DISPLAY_HEIGHT)
    return;
  if (x >= DISPLAY_WIDTH)
    return;

  uint16_t x_end = x + length - 1;
  if (x_end >= DISPLAY_WIDTH)
    x_end = DISPLAY_WIDTH - 1;

  _st7789_set_address_window(x, y, x_end, y);

  DC_HIGH();
  CS_LOW();

  for (uint16_t i = x; i <= x_end; i++) {
    _spi_write(color >> 8);
    _spi_write(color & 0xFF);
  }

  CS_HIGH();
}

/**
 * @brief Рисование вертикальной линии
 * @param x X координата
 * @param y Начальная Y координата
 * @param length Длина линии
 * @param color Цвет линии
 */
void st7789_draw_vline(uint16_t x, uint16_t y, uint16_t length,
                       uint16_t color) {
  // Проверка границ
  if (x >= DISPLAY_WIDTH)
    return;
  if (y >= DISPLAY_HEIGHT)
    return;

  uint16_t y_end = y + length - 1;
  if (y_end >= DISPLAY_HEIGHT)
    y_end = DISPLAY_HEIGHT - 1;

  _st7789_set_address_window(x, y, x, y_end);

  DC_HIGH();
  CS_LOW();

  for (uint16_t i = y; i <= y_end; i++) {
    _spi_write(color >> 8);
    _spi_write(color & 0xFF);
  }

  CS_HIGH();
}

void st7789_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                      uint16_t color) {
  // Оптимизация для горизонтальных и вертикальных линий
  if (y0 == y1) {
    if (x0 < x1)
      st7789_draw_hline(x0, y0, x1 - x0 + 1, color);
    else
      st7789_draw_hline(x1, y0, x0 - x1 + 1, color);
    return;
  }

  if (x0 == x1) {
    if (y0 < y1)
      st7789_draw_vline(x0, y0, y1 - y0 + 1, color);
    else
      st7789_draw_vline(x0, y1, y0 - y1 + 1, color);
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
      _st7789_set_address_window(x0, y0, x0, y0);
      _st7789_write_data_16(color);
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