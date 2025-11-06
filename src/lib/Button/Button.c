#include "Button.h"

volatile bool button_pressed = false;

void button_init(void) {
    DDRD &= ~(1 << PD2);     // PD2 как вход
    PORTD |= (1 << PD2);     // внутренняя подтяжка (HIGH = отжата)

    EICRA |= (1 << ISC01);   // прерывание по спадающему фронту (нажатие)
    EICRA &= ~(1 << ISC00);
    EIMSK |= (1 << INT0);    // разрешить INT0
    sei();                   // разрешить глобальные прерывания
}

ISR(INT0_vect) {
    button_pressed = true;   // флаг для обработки в основном цикле
}