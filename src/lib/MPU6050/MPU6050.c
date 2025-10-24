#include "../I2C/I2C.h"
#include "../MPU6050/MPU6050.h"
#include <avr/io.h>
#include <util/delay.h>

void mpu6050_init(void) {
  i2c_init();

  // Отправляем: START + ADDR(W) + REG + DATA + STOP
  if (i2c_start(MPU6050_ADDR))
    goto error;
  if (i2c_write(MPU6050_REG_PWR_MGMT_1))
    goto error;
  if (i2c_write(0x00))
    goto error; // Wake up
  i2c_stop();
  int16_t gx_off, gy_off, gz_off;
  mpu6050_calibrate_gyro(&gx_off, &gy_off, &gz_off);
  mpu6050_set_gyro_offsets(gx_off, gy_off, gz_off);
  return;

error:
  i2c_stop(); // На всякий случай
              // Можно добавить обработку ошибки (мигание LED и т.п.)
}

static void mpu6050_read_burst(uint8_t reg, uint8_t *buf, uint8_t len) {
  // Запись адреса регистра БЕЗ STOP
  if (i2c_start(MPU6050_ADDR))
    goto error;
  if (i2c_write(reg))
    goto error;

  // Repeated START для чтения
  if (i2c_start(MPU6050_ADDR | 0x01))
    goto error;

  for (uint8_t i = 0; i < len - 1; i++) {
    buf[i] = i2c_read_ack();
  }
  buf[len - 1] = i2c_read_nack();
  i2c_stop();
  return;

error:
  i2c_stop();
  // При ошибке заполняем нулями (или обрабатываем иначе)
  for (uint8_t i = 0; i < len; i++)
    buf[i] = 0;
}

void mpu6050_read_accel(float *ax, float *ay, float *az) {
  uint8_t buf[6];
  mpu6050_read_burst(MPU6050_REG_ACCEL_XOUT_H, buf, 6);

  // Данные в формате Big-Endian, знаковые 16-битные
  int16_t x = (buf[0] << 8) | buf[1];
  int16_t y = (buf[2] << 8) | buf[3];
  int16_t z = (buf[4] << 8) | buf[5];

  // Масштаб: по умолчанию ±2g → 16384 LSB/g
  const float accel_scale = 16384.0f;
  *ax = (float)x / accel_scale;
  *ay = (float)y / accel_scale;
  *az = (float)z / accel_scale;
}

static int16_t gyro_offset_x = 0;
static int16_t gyro_offset_y = 0;
static int16_t gyro_offset_z = 0;

void mpu6050_set_gyro_offsets(int16_t x, int16_t y, int16_t z) {
  gyro_offset_x = x;
  gyro_offset_y = y;
  gyro_offset_z = z;
}

void mpu6050_read_gyro(float *gx, float *gy, float *gz) {
  uint8_t buf[6];
  mpu6050_read_burst(MPU6050_REG_GYRO_XOUT_H, buf, 6);

  int16_t raw_x = ((buf[0] << 8) | buf[1]) - gyro_offset_x;
  int16_t raw_y = ((buf[2] << 8) | buf[3]) - gyro_offset_y;
  int16_t raw_z = ((buf[4] << 8) | buf[5]) - gyro_offset_z;

  const float gyro_scale = 131.0f;
  *gx = (float)raw_x / gyro_scale;
  *gy = (float)raw_y / gyro_scale;
  *gz = (float)raw_z / gyro_scale;
}

void mpu6050_calibrate_gyro(int16_t *gx_offset, int16_t *gy_offset,
                            int16_t *gz_offset) {
  const int samples = 1000;
  int32_t sum_x = 0, sum_y = 0, sum_z = 0;

  for (int i = 0; i < samples; i++) {
    uint8_t buf[6];
    mpu6050_read_burst(MPU6050_REG_GYRO_XOUT_H, buf, 6);
    sum_x += (buf[0] << 8) | buf[1];
    sum_y += (buf[2] << 8) | buf[3];
    sum_z += (buf[4] << 8) | buf[5];
    _delay_ms(2); // небольшая пауза
  }

  *gx_offset = sum_x / samples;
  *gy_offset = sum_y / samples;
  *gz_offset = sum_z / samples;
}