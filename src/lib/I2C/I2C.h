#ifndef I2C_H
#define I2C_H

#include <stdint.h>

#define TWI_FREQ 400000UL // 400 kHz

void i2c_init(void);
uint8_t i2c_start(uint8_t address);
void i2c_stop(void);
uint8_t i2c_write(uint8_t data);
uint8_t i2c_read_ack(void);
uint8_t i2c_read_nack(void);

#endif