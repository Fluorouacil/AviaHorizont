#include "./lib/MPU6050/MPU6050.h"
#include "./lib/ST7735S/ST7735S.h"
#include "./lib/Button/Button.h"
#include "./lib/Screen/screen.h"
#include "./lib/UART/uart.h"
#include <math.h>
#include <util/delay.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>


extern const Screen ST7735S_SCREEN;
static const Screen* screen = &ST7735S_SCREEN;

// Глобальные цвета
static const Color WHITE = {255, 255, 255};
static const Color BLACK = {0, 0, 0};
static const Color SKY_BLUE = {0, 0, 255};
static const Color EARTH_BROWN = {101, 67, 33};
static const Color YELLOW = {255, 255, 0};

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

typedef enum {
    MODE_PITCH_ONLY,  // Только тангаж
    MODE_ROLL_ONLY    // Только крен
} DisplayMode;

static float roll_angle = 0.0f;
static float pitch_angle = 0.0f;
static DisplayMode current_mode = MODE_ROLL_ONLY;
volatile bool update_ready = false;
static bool roll_first_draw = true;
static bool pitch_first_draw = true;
static int pitch_last_horizon_y = -1;

ISR(TIMER1_COMPA_vect) {
    update_ready = true;
}

const Color white = {255, 255, 255};
const Color black = {0, 0, 0};

void fill_screen(const Color *color) {
    st7735s_fill_screen(RGB565(color->red, color->green, color->blue));
}

void draw_roll_ui(const Screen* scr) {
    const int R = (int)(0.35f * scr->width);
    const int cy = R + (int)(0.05f * scr->height);
    const int cx = scr->width / 2;

    const int tick_len = (int)(0.022f * scr->width);
    const int label_dist = tick_len + (int)(0.03f * scr->width);
    const uint8_t font_size = 1;

    const int MIN_COORD = -75;
    const int MAX_COORD = 75;
    const int steps = 61;

    int prev_x = -1, prev_y = -1;
    for (int i = 0; i <= steps; ++i) {
        float coord_deg = MIN_COORD + (MAX_COORD - MIN_COORD) * i / (float)steps;
        float rad = coord_deg * (M_PI / 180.0f);

        int x = cx - (int)(R * sinf(rad));
        int y = cy - (int)(R * cosf(rad));

        if (x < 0 || x >= scr->width || y < 0 || y >= scr->height) {
            prev_x = -1;
            continue;
        }

        if (prev_x != -1) {
            Point2D p1 = make_point(prev_x, prev_y);
            Point2D p2 = make_point(x, y);
            scr->draw_line(&p1, &p2, &WHITE);
        }
        prev_x = x;
        prev_y = y;
    }

    static const int coord_angles[] = {-60, -30, 0, 30, 60};
    const int n = sizeof(coord_angles) / sizeof(coord_angles[0]);

    for (int i = 0; i < n; ++i) {
        int coord_deg = coord_angles[i];
        float rad = coord_deg * (M_PI / 180.0f);

        int x_c = cx - (int)(R * sinf(rad));
        int y_c = cy - (int)(R * cosf(rad));

        float nx = -sinf(rad);
        float ny = -cosf(rad);

        // Штрих
        int x_end = x_c + (int)(nx * tick_len);
        int y_end = y_c + (int)(ny * tick_len);
        Point2D p1 = make_point(x_c, y_c);
        Point2D p2 = make_point(x_end, y_end);
        scr->draw_line(&p1, &p2, &WHITE);

        // Подписи
        char buf[6];
        int label_x, label_y;

        if (coord_deg == -60) {
            strcpy(buf, "-30");
            label_x = x_c + (int)(nx * label_dist);
            label_y = y_c + (int)(ny * label_dist);
        } else if (coord_deg == 60) {
            strcpy(buf, "+30");
            label_x = x_c + (int)(nx * label_dist);
            label_y = y_c + (int)(ny * label_dist);
        } else if (coord_deg == 0) {
            strcpy(buf, "0");
            label_x = x_c + (int)(nx * label_dist);
            label_y = y_c + (int)(ny * label_dist);
        } else if (coord_deg == -30) {
            strcpy(buf, "-60");
            float ext_factor = 1.8f;
            int ext_x = x_c + (int)(nx * label_dist * ext_factor);
            int ext_y = y_c + (int)(ny * label_dist * ext_factor);
            label_x = ext_x;
            label_y = ext_y + (int)(0.08f * scr->height);
        } else if (coord_deg == 30) {
            strcpy(buf, "+60");
            float ext_factor = 1.8f;
            int ext_x = x_c + (int)(nx * label_dist * ext_factor);
            int ext_y = y_c + (int)(ny * label_dist * ext_factor);
            label_x = ext_x;
            label_y = ext_y + (int)(0.08f * scr->height);
        } else {
            continue;
        }

        if (label_x < 2 || label_x > scr->width - 20 || label_y < 2 || label_y > scr->height - 12) continue;

        int text_w;
        if (strcmp(buf, "0") == 0) {
            text_w = 6;
        } else if (buf[1] == '3' || buf[1] == '6') {
            text_w = 10;
        } else {
            text_w = 8;
        }

        float asym_factor = 1.0f;
        if (coord_deg == 60) asym_factor = 3.8f;
        else if (coord_deg == -60) asym_factor = 1.1f;
        else if (coord_deg == 0) asym_factor = 1.5f;
        else if (coord_deg == 30) asym_factor = 4.2f;
        else if (coord_deg == -30) asym_factor = 0.7f;

        label_x -= (int)(text_w * asym_factor / 2.0f);

        if (coord_deg == -60 || coord_deg == 0 || coord_deg == 60) label_y -= 2;
        if (coord_deg == -30 || coord_deg == 0 || coord_deg == 30) label_y -= 6;

        if (scr->draw_string) {
            scr->draw_string(label_x, label_y, buf, &WHITE, font_size);
        }
    }
}

void draw_roll_mode(const Screen* scr, float roll_rad) {
    static int prev_x1 = 0, prev_y1 = 0, prev_x2 = 0, prev_y2 = 0;

    if (roll_first_draw) {
        scr->clear(&BLACK);
        draw_roll_ui(scr);
        roll_first_draw = false;

        const int cx = scr->width / 2;
        const int cy = scr->height / 2;
        const float len = 40.0f;

        prev_x1 = cx + (int)(len * cosf(roll_rad));
        prev_y1 = cy - (int)(len * sinf(roll_rad));
        prev_x2 = cx - (int)(len * cosf(roll_rad));
        prev_y2 = cy + (int)(len * sinf(roll_rad));
    }

    const int cx = scr->width / 2;
    const int cy = scr->height / 2;
    const float len = 40.0f;

    int x1 = cx + (int)(len * cosf(roll_rad));
    int y1 = cy - (int)(len * sinf(roll_rad));
    int x2 = cx - (int)(len * cosf(roll_rad));
    int y2 = cy + (int)(len * sinf(roll_rad));

    if (!roll_first_draw) {
        int min_x = MIN(prev_x1, prev_x2);
        int max_x = MAX(prev_x1, prev_x2);
        int min_y = MIN(prev_y1, prev_y2);
        int max_y = MAX(prev_y1, prev_y2);

        const int pad = 2;
        min_x = MAX(0, min_x - pad);
        min_y = MAX(0, min_y - pad);
        max_x = MIN(scr->width - 1, max_x + pad);
        max_y = MIN(scr->height - 1, max_y + pad);

        int w = max_x - min_x + 1;
        int h = max_y - min_y + 1;

        scr->fill_rect(min_x, min_y, w, h, &BLACK);
    }

    Point2D p1 = make_point(x1, y1);
    Point2D p2 = make_point(x2, y2);
    scr->draw_line(&p1, &p2, &WHITE);

    prev_x1 = x1;
    prev_y1 = y1;
    prev_x2 = x2;
    prev_y2 = y2;
}

float calculate_roll_from_accel(float ax, float ay, float az) {
    return atan2f(ay, az);
}

void draw_pitch_ui_full(const Screen* scr, float pitch_rad) {
    const float scale = 60.0f;
    const int centerY = scr->height / 2;

    const int long_len  = 22;
    const int short_len = 10;
    const int text_offset = 5;
    const int text_y_shift = -3;

    for (int deg = -60; deg <= 60; deg += 5) {
        float rad = deg * (M_PI / 180.0f);
        int y = centerY - (int)(rad * scale);

        if (y < -5 || y >= scr->height + 5) continue;

        int len = (deg % 15 == 0) ? long_len : short_len;

        scr->draw_hline(scr->width / 2 - len, y, len * 2, &WHITE);

        if (deg % 15 != 0) continue;

        char buf[5];
        int idx = 0;
        int a = (deg < 0) ? -deg : deg;

        if (a >= 10) buf[idx++] = '0' + (a / 10);
        buf[idx++] = '0' + (a % 10);
        buf[idx++] = 176; // '°'
        buf[idx] = '\0';

        int text_w = idx * 7;
        int tx_left  = scr->width / 2 - len - text_offset - text_w;
        int tx_right = scr->width / 2 + len + text_offset;

        if (scr->draw_string) {
            scr->draw_string(tx_left,  y + text_y_shift, buf, &YELLOW, 1);
            scr->draw_string(tx_right, y + text_y_shift, buf, &YELLOW, 1);
        }
    }

    // Выделение 0°
    int y0 = centerY;
    if (y0 >= 0 && y0 < scr->height) {
        scr->draw_hline(scr->width / 2 - long_len, y0, long_len * 2, &WHITE);
        scr->draw_vline(scr->width / 2 - long_len,     y0 - 2, 5, &WHITE);
        scr->draw_vline(scr->width / 2 + long_len - 1, y0 - 2, 5, &WHITE);
    }
}

void draw_pitch_ui(const Screen* scr, float pitch_rad) {
    static int last_pitch_deg = 1000;

    int pitch_deg = (int)roundf(pitch_rad * 180.0f / M_PI);
    pitch_deg = ((pitch_deg + 2) / 5) * 5;

    if (pitch_first_draw) {
        draw_pitch_ui_full(scr, pitch_deg * M_PI / 180.0f);
        pitch_first_draw = false;
        last_pitch_deg = pitch_deg;
        return;
    }

    if (pitch_deg == last_pitch_deg) return;

    draw_pitch_ui_full(scr, pitch_deg * M_PI / 180.0f);
    last_pitch_deg = pitch_deg;
}

void update_sky_ground(const Screen* scr, float pitch_rad) {
    float max_rad = 45.0f * (M_PI / 180.0f);
    if (pitch_rad > max_rad) pitch_rad = max_rad;
    if (pitch_rad < -max_rad) pitch_rad = -max_rad;
    pitch_rad = roundf(pitch_rad / (5.0f * M_PI / 180.0f)) * (5.0f * M_PI / 180.0f);

    int horizon_y = (int)((float)(scr->height / 2) - pitch_rad * 60.0f);
    if (horizon_y < 0) horizon_y = 0;
    if (horizon_y > scr->height) horizon_y = scr->height;

    if (pitch_last_horizon_y == -1) {
        scr->fill_rect(0, 0, scr->width, horizon_y, &EARTH_BROWN);
        scr->fill_rect(0, horizon_y, scr->width, scr->height - horizon_y, &SKY_BLUE);
        pitch_last_horizon_y = horizon_y;
        return;
    }

    if (horizon_y > pitch_last_horizon_y) {
        int dy = horizon_y - pitch_last_horizon_y;
        scr->fill_rect(0, pitch_last_horizon_y, scr->width, dy, &EARTH_BROWN);
    } else if (horizon_y < pitch_last_horizon_y) {
        int dy = pitch_last_horizon_y - horizon_y;
        scr->fill_rect(0, horizon_y, scr->width, dy, &SKY_BLUE);
    }

    pitch_last_horizon_y = horizon_y;
}

void draw_pitch_mode(const Screen* scr, float pitch_rad) {
    static float last_pitch_for_draw = 1000.0f;

    if (pitch_first_draw) {
        // Принудительно перерисуем всё
        draw_pitch_ui_full(scr, pitch_rad);
        update_sky_ground(scr, pitch_rad);
        last_pitch_for_draw = pitch_rad;
        pitch_first_draw = false;
        return;
    }

    bool pitch_changed = (fabsf(pitch_rad - last_pitch_for_draw) > 0.05f);
    if (pitch_changed) {
        update_sky_ground(scr, pitch_rad);
        last_pitch_for_draw = pitch_rad;
    }

    draw_pitch_ui(scr, pitch_rad);

    const int cx = scr->width / 2;
    const int cy = scr->height / 2;
    const int len = 40;
    Point2D p1 = make_point(cx - len, cy);
    Point2D p2 = make_point(cx + len, cy);
    scr->draw_line(&p1, &p2, &WHITE);
}

int main(void) {
    st7735s_init();
    mpu6050_init();
    button_init();

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
        while (!update_ready) {}
        update_ready = false;

        float gx, gy, gz, ax, ay, az;
        mpu6050_read_gyro(&gx, &gy, &gz);
        mpu6050_read_accel(&ax, &ay, &az);

        // Обновление roll
        float roll_gyro = roll_angle + (gx * (M_PI / 180.0f)) * dt;
        float roll_accel = atan2f(ay, az);
        float acc_mag = sqrtf(ax*ax + ay*ay + az*az);
        float acc_error = fabsf(acc_mag - 1.0f);
        float alpha = (acc_error < 0.05f) ? 0.90f : (acc_error < 0.15f) ? 0.95f : 0.985f;
        roll_angle = alpha * roll_gyro + (1.0f - alpha) * roll_accel;
        if (roll_angle > M_PI) roll_angle -= 2.0f * M_PI;
        if (roll_angle < -M_PI) roll_angle += 2.0f * M_PI;

        // Обновление pitch
        pitch_angle = atan2f(-ax, sqrtf(ay*ay + az*az));

        // Кнопка
        if (button_pressed) {
            button_pressed = false;
            _delay_ms(30);
            if ((PIND & (1 << PD2)) == 0) {
                current_mode = (current_mode == MODE_PITCH_ONLY) 
                               ? MODE_ROLL_ONLY 
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