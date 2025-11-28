#ifndef UART_H
#define UART_H

#include <avr/io.h>

void uart_init(void);
void uart_putc(uint8_t c);
void uart_send_float(float f);
void send_attitude_packet(float roll, float pitch, uint8_t mode);

#endif