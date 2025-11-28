#include "uart.h"

void uart_init_send(void) {
  UBRR0H = 0;
  UBRR0L = 16;
  UCSR0A = 0;
  UCSR0B = (1 << TXEN0);
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_init_read(void) {
  UBRR0H = 0;
  UBRR0L = 16;
  UCSR0A = 0;
  UCSR0B = (1 << RXEN0) | (1 << RXCIE0);
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_putc(uint8_t c) {
  while (!(UCSR0A & (1 << UDRE0)))
    ;
  UDR0 = c;
}

void uart_send_float(float f) {
  union {
    float f;
    uint8_t b[4];
  } u = {.f = f};
  for (int i = 0; i < 4; i++)
    uart_putc(u.b[i]);
}

void send_attitude_packet(float roll, float pitch, uint8_t mode) {
  uart_putc(0xFE); // STX
  uart_send_float(roll);
  uart_send_float(pitch);
  uart_putc(mode);
  uart_putc(0xFF); // ETX
}