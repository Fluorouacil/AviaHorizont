#include "./lib/Button/Button.h"
#include "./lib/MPU6050/MPU6050.h"
#include "./lib/UART/uart.h"

#include <stdio.h>

#include "mcu.h"

typedef enum {
  MODE_PITCH_ONLY, // Только тангаж
  MODE_ROLL_ONLY   // Только крен
} DisplayMode;

static float roll_angle = 0.0f;
static float pitch_angle = 0.0f;
static DisplayMode current_mode = MODE_ROLL_ONLY;

volatile bool update_ready = false;

ISR(TIMER1_COMPA_vect) { update_ready = true; }

float calculate_roll_from_accel(float ax, float ay, float az) {
  return atan2f(ay, az);
}

int main(void) {
  st7735s_init();
  mpu6050_init();
  button_init();
  uart_init_send();

  screen->clear(&BLACK);

  const float dt = 0.01f;

  TCCR1A = 0;
  TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10);
  OCR1A = 7499;
  TIMSK1 = (1 << OCIE1A);
  TCNT1 = 0;
  sei();

  update_ready = true;

  while (1) {
    while (!update_ready)
      ;
    update_ready = false;

    float gx, gy, gz, ax, ay, az;
    mpu6050_read_gyro(&gx, &gy, &gz);
    mpu6050_read_accel(&ax, &ay, &az);

    // Обновление roll
    float roll_gyro = roll_angle + (gx * (M_PI / 180.0f)) * dt;
    float roll_accel = atan2f(ay, az);
    float acc_mag = sqrtf(ax * ax + ay * ay + az * az);
    float acc_error = fabsf(acc_mag - 1.0f);
    float alpha = (acc_error < 0.05f)   ? 0.90f
                  : (acc_error < 0.15f) ? 0.95f
                                        : 0.985f;
    roll_angle = alpha * roll_gyro + (1.0f - alpha) * roll_accel;
    if (roll_angle > M_PI)
      roll_angle -= 2.0f * M_PI;
    if (roll_angle < -M_PI)
      roll_angle += 2.0f * M_PI;

    // Обновление pitch
    pitch_angle = atan2f(-ax, sqrtf(ay * ay + az * az));

    // Кнопка
    if (button_pressed) {
      button_pressed = false;
      _delay_ms(30);
      if ((PIND & (1 << PD2)) == 0) {
        current_mode = (current_mode == MODE_PITCH_ONLY) ? MODE_ROLL_ONLY
                                                         : MODE_PITCH_ONLY;
        screen->clear(&BLACK);
        roll_first_draw = true;
        pitch_first_draw = true;
        pitch_last_horizon_y = -1;
      }
    }

    uint8_t mode_bit = (current_mode == MODE_PITCH_ONLY) ? 1 : 0;
    send_attitude_packet(roll_angle, pitch_angle, mode_bit);

    // Рендер
    if (current_mode == MODE_PITCH_ONLY) {
      draw_pitch_mode(screen, pitch_angle);
    } else {
      draw_roll_mode(screen, roll_angle);
    }
  }
}