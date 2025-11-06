#include "./lib/MPU6050/MPU6050.h"
#include "./lib/ST7789/ST7789.h"
#include "./lib/Button/Button.h"
#include <math.h>
#include <util/delay.h>
#include <stdbool.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

typedef enum {
    MODE_PITCH_ONLY,  // Только тангаж
    MODE_ROLL_ONLY    // Только крен (чёрный фон)
} DisplayMode;

static float roll_angle = 0.0f;
static bool first_draw = true;
static int prev_x1 = 0, prev_y1 = 0;
static int prev_x2 = 0, prev_y2 = 0;
static float pitch_angle = 0.0f;
static float last_pitch_for_draw = 1000.0f; // заведомо отличное
static DisplayMode current_mode = MODE_ROLL_ONLY;


void draw_line(const ScreenPoint *sp1, const ScreenPoint *sp2, const Color *const color) {
    int16_t x0 = sp1->coordinates[0];
    int16_t y0 = sp1->coordinates[1];
    int16_t x1 = sp2->coordinates[0];
    int16_t y1 = sp2->coordinates[1];

    st7789_draw_line((uint16_t)x0, (uint16_t)y0, (uint16_t)x1, (uint16_t)y1,
                     RGB565(color->red, color->green, color->blue));
}

const Screen screen = {
    .width = DISPLAY_WIDTH,
    .height = DISPLAY_HEIGHT,
    .draw_line = draw_line
};

const Color white = {255, 255, 255};
const Color black = {0, 0, 0};

void fill_screen(const Color *color) {
    st7789_fill_screen(RGB565(color->red, color->green, color->blue));
}

void draw_roll_mode(void) {
    if (first_draw) {
        fill_screen(&black);
        first_draw = false;
    }

    const int cx = DISPLAY_WIDTH / 2;
    const int cy = DISPLAY_HEIGHT / 2;
    const float len = 40.0f;

    int x1 = cx + (int)(len * cosf(roll_angle));
    int y1 = cy - (int)(len * sinf(roll_angle));
    int x2 = cx - (int)(len * cosf(roll_angle));
    int y2 = cy + (int)(len * sinf(roll_angle));

    // Стереть старую линию
    if (!first_draw) {
        ScreenPoint old_p1 = {{prev_x1, prev_y1}};
        ScreenPoint old_p2 = {{prev_x2, prev_y2}};
        screen.draw_line(&old_p1, &old_p2, &black);
    }

    // Нарисовать новую
    ScreenPoint p1 = {{x1, y1}};
    ScreenPoint p2 = {{x2, y2}};
    screen.draw_line(&p1, &p2, &white);

    prev_x1 = x1; prev_y1 = y1;
    prev_x2 = x2; prev_y2 = y2;
}

float calculate_roll_from_accel(float ax, float ay, float az) {
    return atan2f(ay, az);
}

void draw_sky_ground(float pitch) {
    // Ограничиваем тангаж разумными пределами (например, ±30 градусов)
    const float max_pitch_rad = 30.0f * (M_PI / 180.0f);
    if (pitch > max_pitch_rad) pitch = max_pitch_rad;
    if (pitch < -max_pitch_rad) pitch = -max_pitch_rad;

    // Горизонт в пикселях: смещение от центра
    // Коэффициент масштабирования: 1 рад ≈ (DISPLAY_HEIGHT/2) пикселей?
    // Но лучше подобрать эмпирически, например: 1 рад = 60 px
    const float scale = 60.0f; // пикселей на радиан
    int horizon_y = (int)((float)(DISPLAY_HEIGHT / 2) + pitch * scale);

    // Ограничиваем, чтобы не выйти за границы
    if (horizon_y < 0) horizon_y = 0;
    if (horizon_y > DISPLAY_HEIGHT) horizon_y = DISPLAY_HEIGHT;

    // Заливаем НЕБО (сверху до горизонта) — чёрный? синий?
    st7789_fill_rect(0, 0, DISPLAY_WIDTH, horizon_y, RGB565(0, 0, 255)); // или COLOR_BLUE

    // Заливаем ЗЕМЛЮ (от горизонта до низа) — коричневый/зелёный?
    st7789_fill_rect(0, horizon_y, DISPLAY_WIDTH, DISPLAY_HEIGHT - horizon_y, RGB565(101, 67, 33)); // коричневый
}

void draw_pitch_mode(void) {
    // Обновляем фон при изменении тангажа
    bool pitch_changed = (fabsf(pitch_angle - last_pitch_for_draw) > 0.05f);
    if (pitch_changed) {
        draw_sky_ground(pitch_angle);
        last_pitch_for_draw = pitch_angle;
        first_draw = true;
    }

    // Рисуем ГОРИЗОНТАЛЬНУЮ линию по центру (не зависит от крена!)
    const int cx = DISPLAY_WIDTH / 2;
    const int cy = DISPLAY_HEIGHT / 2;
    const int len = 40;

    ScreenPoint p1 = {{cx - len, cy}};
    ScreenPoint p2 = {{cx + len, cy}};
    screen.draw_line(&p1, &p2, &white);
}

int main(void) {
    st7789_init();
    mpu6050_init();
    button_init();

    fill_screen(&black);

    const float dt = 0.01f;

    while (1) {
        float gx, gy, gz, ax, ay, az;
        mpu6050_read_gyro(&gx, &gy, &gz);
        mpu6050_read_accel(&ax, &ay, &az);

        float roll_gyro = roll_angle + (gx * (M_PI / 180.0f)) * dt;
        float roll_accel = atan2f(ay, az);
        float acc_mag = sqrtf(ax*ax + ay*ay + az*az);
        float acc_error = fabsf(acc_mag - 1.0f);
        float alpha = (acc_error < 0.05f) ? 0.90f : (acc_error < 0.15f) ? 0.95f : 0.985f;
        roll_angle = alpha * roll_gyro + (1.0f - alpha) * roll_accel;
        if (roll_angle > M_PI) roll_angle -= 2.0f * M_PI;
        if (roll_angle < -M_PI) roll_angle += 2.0f * M_PI;

        pitch_angle = atan2f(-ax, sqrtf(ay*ay + az*az));

        if (button_pressed) {
            button_pressed = false;
            _delay_ms(30);
            if ((PIND & (1 << PD2)) == 0) { // всё ещё нажата?
                current_mode = (current_mode == MODE_PITCH_ONLY) 
                               ? MODE_ROLL_ONLY 
                               : MODE_PITCH_ONLY;
                fill_screen(&black);
                first_draw = true;
                last_pitch_for_draw = 1000.0f;
            }
        }

        if (current_mode == MODE_PITCH_ONLY) {
            draw_pitch_mode();
        } else {
            draw_roll_mode();
        }

        _delay_ms(30);
    }
}