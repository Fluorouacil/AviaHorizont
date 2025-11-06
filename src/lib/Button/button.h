#ifndef BUTTON_H
#define BUTTON_H

#include <avr/io.h>
#include <stdbool.h>
#include <avr/interrupt.h>

extern volatile bool button_pressed;

void button_init(void);

#endif