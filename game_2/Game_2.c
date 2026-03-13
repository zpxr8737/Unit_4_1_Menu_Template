#include "Game_2.h" 
#include "InputHandler.h"
#include "Menu.h"
#include "LCD.h"
#include "Buzzer.h"
#include "stm32l4xx_hal.h"
#include <stdio.h>
#include <stdlib.h>

extern ST7789V2_cfg_t cfg0;
extern Buzzer_cfg_t buzzer_cfg;

// Frame rate
#define GAME2_FRAME_TIME_MS 50  // ~20 FPS
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320
#define MAX_FRUITS 5
#define MAX_BULLETS 3

// Player car state
static int16_t car_x = SCREEN_WIDTH / 2;
static int16_t car_y = SCREEN_HEIGHT - 40;
static int16_t car_speed = 5;

// Bullet structure
typedef struct {
    int16_t x, y;
    uint8_t active;
} Bullet;

// Fruit structure
typedef struct {
    int16_t x, y;
    int16_t old_x, old_y;  // previous position for erasing
    uint8_t active;
    uint8_t type;  // 0=apple,1=banana,2=cherry
} Fruit;

static Bullet bullets[MAX_BULLETS];
static Fruit fruits[MAX_FRUITS];
static uint32_t score = 0;

// Initialize fruits at random positions
static void InitFruits(void) {
    for (int i = 0; i < MAX_FRUITS; i++) {
        fruits[i].x = rand() % (SCREEN_WIDTH - 20) + 10;
        fruits[i].y = -(rand() % 200);  // Start off-screen
        fruits[i].old_x = fruits[i].x;
        fruits[i].old_y = fruits[i].y;
        fruits[i].active = 1;
        fruits[i].type = rand() % 3;
    }
}

// Fire a bullet from the car
static void FireBullet(void) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
            bullets[i].x = car_x;
            bullets[i].y = car_y - 10;
            bullets[i].active = 1;
            buzzer_tone(&buzzer_cfg, 1500, 20);  // Shooting sound
            HAL_Delay(10);
            buzzer_off(&buzzer_cfg);
            break;
        }
    }
}

// Check collision between bullet and fruit
static uint8_t CheckCollision(Bullet *b, Fruit *f) {
    if (!b->active || !f->active) return 0;
    if (b->x >= f->x - 8 && b->x <= f->x + 8 &&
        b->y >= f->y - 8 && b->y <= f->y + 8) {
        return 1;
    }
    return 0;
}

MenuState Game2_Run(void) {
    car_x = SCREEN_WIDTH / 2;
    score = 0;

    // Initialize fruits and bullets
    InitFruits();
    for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = 0;

    MenuState exit_state = MENU_STATE_HOME;

    while (1) {
        uint32_t frame_start = HAL_GetTick();

        // Read input
        Input_Read();

        // Exit game
        if (current_input.btn3_pressed) {
            exit_state = MENU_STATE_HOME;
            break;
        }

        // Move car left/right
        int16_t old_car_x = car_x;
        if (current_input.btn_left) car_x -= car_speed;
        if (current_input.btn_right) car_x += car_speed;
        if (car_x < 10) car_x = 10;
        if (car_x > SCREEN_WIDTH - 10) car_x = SCREEN_WIDTH - 10;

        // Shoot bullet
        if (current_input.btn1_pressed) {
            FireBullet();
        }

        // Update bullets
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                // Erase old bullet
                LCD_drawRect(bullets[i].x - 1, bullets[i].y, 2, 6, 0);  // small vertical erase
                bullets[i].y -= 8;
                if (bullets[i].y < 0) bullets[i].active = 0;
            }
        }

        // Update fruits
        for (int i = 0; i < MAX_FRUITS; i++) {
            if (fruits[i].active) {
                // Erase old fruit
                LCD_drawRect(fruits[i].old_x - 4, fruits[i].old_y - 4, 8, 8, 0);

                fruits[i].old_x = fruits[i].x;
                fruits[i].old_y = fruits[i].y;

                fruits[i].y += 3;  // Fruit speed
                if (fruits[i].y > SCREEN_HEIGHT) {
                    fruits[i].y = -20;
                    fruits[i].x = rand() % (SCREEN_WIDTH - 20) + 10;
                    fruits[i].type = rand() % 3;
                    fruits[i].old_x = fruits[i].x;
                    fruits[i].old_y = fruits[i].y;
                }
            }
        }

        // Check collisions
        for (int i = 0; i < MAX_BULLETS; i++) {
            for (int j = 0; j < MAX_FRUITS; j++) {
                if (CheckCollision(&bullets[i], &fruits[j])) {
                    bullets[i].active = 0;
                    fruits[j].active = 0;
                    score += 10;
                    buzzer_tone(&buzzer_cfg, 2000, 30);  // Hit sound
                    HAL_Delay(20);
                    buzzer_off(&buzzer_cfg);

                    // Respawn fruit
                    fruits[j].x = rand() % (SCREEN_WIDTH - 20) + 10;
                    fruits[j].y = -(rand() % 200);
                    fruits[j].old_x = fruits[j].x;
                    fruits[j].old_y = fruits[j].y;
                    fruits[j].active = 1;
                    fruits[j].type = rand() % 3;
                }
            }
        }

        // Draw track boundaries (only once is fine, or could redraw)
        LCD_drawRect(30, 0, SCREEN_WIDTH - 60, SCREEN_HEIGHT, 1);

        // Draw car
        LCD_printString("A", car_x - 4, car_y, 1, 3);  // Simple car sprite

        // Draw bullets
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                LCD_printString("|", bullets[i].x, bullets[i].y, 1, 2);
            }
        }

        // Draw fruits
        for (int i = 0; i < MAX_FRUITS; i++) {
            if (fruits[i].active) {
                char fruit_char = 'F';
                switch(fruits[i].type) {
                    case 0: fruit_char = 'A'; break;  // apple
                    case 1: fruit_char = 'B'; break;  // banana
                    case 2: fruit_char = 'C'; break;  // cherry
                }
                LCD_printString(&fruit_char, fruits[i].x, fruits[i].y, 1, 2);
            }
        }

        // Display score
        char score_str[32];
        sprintf(score_str, "Score: %lu", score);
        LCD_printString(score_str, 10, 10, 1, 2);

        // Instructions
        LCD_printString("Left/Right: Move", 10, SCREEN_HEIGHT - 40, 1, 1);
        LCD_printString("BTN1: Shoot", 10, SCREEN_HEIGHT - 25, 1, 1);
        LCD_printString("BTN3: Exit", 10, SCREEN_HEIGHT - 10, 1, 1);

        // Frame timing
        uint32_t frame_time = HAL_GetTick() - frame_start;
        if (frame_time < GAME2_FRAME_TIME_MS) {
            HAL_Delay(GAME2_FRAME_TIME_MS - frame_time);
        }
    }

    return exit_state;
}