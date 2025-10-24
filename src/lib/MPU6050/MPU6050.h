#ifndef MPU6050_H
#define MPU6050_H

#include <stdint.h>

// Адрес по умолчанию: AD0 = GND → 0x68 → сдвинутый = 0xD0
// Если AD0 = VCC → 0x69 → 0xD2
#define MPU6050_ADDR 0xD0

#define MPU6050_REG_PWR_MGMT_1 0x6B
#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define MPU6050_REG_GYRO_XOUT_H 0x43

void mpu6050_init(void);
void mpu6050_read_accel(float *ax, float *ay, float *az);
void mpu6050_read_gyro(float *gx, float *gy, float *gz);
void mpu6050_set_gyro_offsets(int16_t x, int16_t y, int16_t z);
void mpu6050_calibrate_gyro(int16_t *gx_offset, int16_t *gy_offset,
                            int16_t *gz_offset);

#endif