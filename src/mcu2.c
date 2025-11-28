#include "./lib/UART/uart.h"

#include <avr/interrupt.h>
#include <avr/io.h>

#include "mcu.h"

// Пакет данных
typedef struct {
  float roll;
  float pitch;
  uint8_t mode; // 0 = мы рисуем pitch, 1 = мы рисуем roll
} AttitudePacket;

// Глобальные для приёма
volatile bool packet_ready = false;
volatile AttitudePacket received = {0};

static uint8_t buf[11]; // WARN
static uint8_t idx = 0;

// Флаги для первого рисования
ISR(USART_RX_vect) {
  uint8_t c = UDR0;

  if (idx == 0 && c != 0xFE) {
    // sync lost — сброс
    idx = 0;
    return;
  }

  buf[idx] = c;
  idx++;

  if (idx == 11) {
    if (buf[10] == 0xFF) {
      // Распаковка
      union {
        uint8_t b[4];
        float f;
      } roll_u, pitch_u;
      for (int i = 0; i < 4; i++) {
        roll_u.b[i] = buf[1 + i];
        pitch_u.b[i] = buf[5 + i];
      }

      // Копируем в volatile-структуру атомарно (или через cli/sei)
      AttitudePacket temp;
      temp.roll = roll_u.f;
      temp.pitch = pitch_u.f;
      temp.mode = buf[9];

      cli();
      received = temp;
      packet_ready = true;
      sei();
    }
    idx = 0;
  } else if (idx > 11) {
    idx = 0; // защита от переполнения
  }
}

int main(void) {
  // Инициализация
  st7735s_init();
  uart_init_read();

  sei(); // разрешить прерывания

  screen->clear(&BLACK);

  while (1) {
    if (packet_ready) {
      cli();
      AttitudePacket pkt = received;
      packet_ready = false;
      sei();

      static uint8_t last_mode = 0xFF;
      if (pkt.mode != last_mode) {
        screen->clear(&BLACK);
        roll_first_draw = true;
        pitch_first_draw = true;
        pitch_last_horizon_y = -1;
        last_mode = pkt.mode;
      }

      if (pkt.mode == 0) {
        // Мы — pitch-экран
        draw_pitch_mode(screen, pkt.pitch);
      } else {
        // Мы — roll-экран
        draw_roll_mode(screen, pkt.roll);
      }
    }
  }
}