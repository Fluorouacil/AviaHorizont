#include "I2C.h"
#include <avr/io.h>
#include <util/twi.h>

// Тайм-аут для ожидания TWINT (подберите под вашу частоту)
#define I2C_TIMEOUT 30000

void i2c_init(void) {
  TWSR = 0; // Предделитель = 1
  TWBR = ((F_CPU / TWI_FREQ) - 16) / 2;
}

static uint8_t i2c_wait_for_completion(void) {
  uint16_t timeout = I2C_TIMEOUT;
  while (!(TWCR & (1 << TWINT))) {
    if (--timeout == 0)
      return 1; // Тайм-аут
  }
  return 0;
}

uint8_t i2c_start(uint8_t address) {
  TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
  if (i2c_wait_for_completion())
    return 1;

  uint8_t status = TWSR & 0xF8;
  if (status != TW_START && status != TW_REP_START)
    return 1;

  TWDR = address;
  TWCR = (1 << TWINT) | (1 << TWEN);
  if (i2c_wait_for_completion())
    return 1;

  status = TWSR & 0xF8;
  if (status == TW_MT_SLA_ACK || status == TW_MR_SLA_ACK)
    return 0;
  else
    return 1;
}

void i2c_stop(void) {
  TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
  // TWSTO автоматически сбрасывается, но не виснем навсегда
  uint16_t timeout = I2C_TIMEOUT;
  while ((TWCR & (1 << TWSTO)) && --timeout) {
    // Ждём завершения STOP
  }
}

uint8_t i2c_write(uint8_t data) {
  TWDR = data;
  TWCR = (1 << TWINT) | (1 << TWEN);
  if (i2c_wait_for_completion())
    return 1;

  return ((TWSR & 0xF8) == TW_MT_DATA_ACK) ? 0 : 1;
}

uint8_t i2c_read_ack(void) {
  TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
  if (i2c_wait_for_completion())
    return 0; // Возвращаем 0 при ошибке
  return TWDR;
}

uint8_t i2c_read_nack(void) {
  TWCR = (1 << TWINT) | (1 << TWEN);
  if (i2c_wait_for_completion())
    return 0;
  return TWDR;
}