#include "./lib/MPU6050/MPU6050.h"
#include "./lib/ST7789/ST7789.h"
#include <math.h>
#include <util/delay.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

static float roll_angle = 0.0f;

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

void draw_horizon_line(void) {
    const int cx = screen.width / 2;  
    const int cy = screen.height / 2;
    const float len = 60.0f; // фиксированная длина

    int x1 = cx + (int)(len * cosf(roll_angle));
    int y1 = cy + (int)(len * sinf(roll_angle));
    int x2 = cx - (int)(len * cosf(roll_angle));
    int y2 = cy - (int)(len * sinf(roll_angle));

    ScreenPoint p1 = {{x1, y1}};
    ScreenPoint p2 = {{x2, y2}};
    screen.draw_line(&p1, &p2, &white);
}

float calculate_roll_from_accel(float ax, float ay, float az) {
    return atan2f(ay, az);
}

int main(void) {
    st7789_init();
    mpu6050_init();
    
    fill_screen(&black);

    const float dt = 0.1f;

    while (1) {
        float gx, gy, gz;
        float ax, ay, az;
        
        mpu6050_read_gyro(&gx, &gy, &gz);
        mpu6050_read_accel(&ax, &ay, &az);

        float roll_gyro = roll_angle + (gx * (M_PI / 180.0f)) * dt;

        float roll_accel = calculate_roll_from_accel(ax, ay, az);

        float acc_mag = sqrtf(ax*ax + ay*ay + az*az);   
        float acc_error = fabsf(acc_mag - 1.0f); 

        float alpha;
        if (acc_error < 0.05f) {
            alpha = 0.90f;
        } else if (acc_error < 0.15f) {
            alpha = 0.95f; 
        } else {
            alpha = 0.985f;
        }

        roll_angle = alpha * roll_gyro + (1.0f - alpha) * roll_accel;

        if (roll_angle > M_PI) roll_angle -= 2.0f * M_PI;
        if (roll_angle < -M_PI) roll_angle += 2.0f * M_PI;

        fill_screen(&black);
        draw_horizon_line();
        _delay_ms(10);
    }

    return 0;
}