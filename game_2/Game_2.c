#include "Game_2.h"
#include "InputHandler.h"
#include "Menu.h"
#include "LCD.h"
#include "Buzzer.h"
#include "Joystick.h"
#include "stm32l4xx_hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

extern ST7789V2_cfg_t cfg0;
extern Buzzer_cfg_t buzzer_cfg;
extern Joystick_cfg_t joystick_cfg;
extern Joystick_t joystick_data;

// fruit sprites
const uint8_t appleSprite[16][16] = {
{255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255},
{255,255,255,255,255,255,255,255, 11, 11, 11,255,255,255,255,255},
{255,255,255,  1,  1,  1,255, 11,  3,  7,  0,255,255,255,255,255},
{255,255,255,  1,  6,  6,  1,  7,  7,  3,  0,  4,255,255,255,255},
{255,255,255,  1,  6,  6,  4,  7,  3,  0,  4,  4,  4,255,255,255},
{255,255,255,  1,  1,  2,  2,  4,  4,  4,  4,  4,  4,  4,255,255},
{255,255,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,255,255},
{255,255,  4,  4,  4,  5,  5,  4,  5,  5,  4,  4,  4,  1,255,255},
{255,255,  1,  4,  4,  5,  5,  4,  4,  4,  4,  4,  4,  1,255,255},
{255,255,  1,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  1,255,255},
{255,255,255,  1,  4,  4,  4,  4,  4,  4,  4,  4,  4,  1,255,255},
{255,255,255,  1,  4,  4,  4,  4,  4,  4,  4,  4,  4,  1,255,255},
{255,255,255,255,  1,  4,  4,  4,  4,  4,  4,  4,  1,255,255,255},
{255,255,255,255,255,  1,  4,  4,  4,  1,  1,  1,255,255,255,255},
{255,255,255,255,255,255,  1,  1,  1,255,255,255,255,255,255,255},
{255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255}
};

const uint8_t orangeSprite[16][16] = {
{255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255},
{255,255,255,255,255,255,255,255, 11, 11, 11,255,255,255,255,255},
{255,255,255,255,255,255,255, 11,  3,  7,  0,255,255,255,255,255},
{255,255,255,  1,  1,  1,  1,  7,  7,  3,  0,255,255,255,255,255},
{255,255,255,  1,  6,  6,  4,  7,  7,  0,  4,  4,255,255,255,255},
{255,255,255,  1,  1,  4,  2,  4,  4,  6,  8,  6,  4,255,255,255},
{255,255,255,  4,  8,  6,  4,  4,  6,  8,  8,  8,  4,255,255,255},
{255,255,  4,  8,  8,  8,  8,  8,  8,  8,  8,  8,  6,  4,255,255},
{255,255,  4,  8,  8,  8,  8,  8,  8,  8,  8,  8,  6,  4,255,255},
{255,255,  1,  8,  8,  8,  8,  8,  8,  8,  8,  6,  5,  1,255,255},
{255,255,  1,  6,  8,  8,  8,  8,  8,  6,  5,  5,  4,  1,255,255},
{255,255,255,  1,  5,  6,  6,  6,  5,  5,  4,  4,  1,255,255,255},
{255,255,255,  1,  5,  5,  4,  4,  4,  4,  4,  5,  1,255,255,255},
{255,255,255,255,  1,  1,  5,  4,  4,  5,  1,  1,255,255,255,255},
{255,255,255,255,255,255,  1,  1,  1,  1,255,255,255,255,255,255},
{255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255}
};

const uint8_t watermelonSprite[16][16] = {
{255,255,255,255,255,255,  3,  3,  3,  3,  3,255,255,  3,  3,255},
{255,255,255,255,  3,  3,  7,  3,  3,  3,  3,  3,  3,  0,  0,  0},
{255,255,255,  3,  7,  3,  3,  7,  7,  7,  3,  3,  7,  3,255,  0},
{255,255,  0,  3,  3,  3,  7,  7,  7,  7,  7,  3,  3,  7,  3,255},
{255,  0,  3,  3,  7,  7,  7,  7,  7,  7,  7,  3,  3,  3,  3,255},
{255,  0,  3,  7,  7,  7,  7,  7,  7,  3,  7,  7,  3,  3,  3},
{  0,  3,  3,  7,  7,  7,  7,  7,  7,  3,  7,  7,  7,  7,  3,  3},
{  0,  0,  3,  7,  7,  7,  7,  7,  3,  7,  7,  7,  7,  3,  0,  0},
{  0,  0,  3,  3,  7,  7,  7,  3,  3,  7,  7,  7,  3,  3,  0,  0},
{  0,  3,  3,  3,  3,  3,  0,  3,  7,  7,  3,  3,  3,  0,  3,  0},
{  0,  3,  3,  3,  3,  0,  3,  3,  3,  3,  3,  3,  3,  0,  3,  0},
{255,  0,  3,  3,  0,  0,  3,  3,  3,  3,  3,  3,  0,  3,  0,255},
{255,  0,  3,  3,  0,  3,  3,  3,  3,  3,  3,  0,  3,  3,  0,255},
{255,255,  0,  3,  3,  3,  3,  3,  3,  3,  0,  0,  3,  0,255,255},
{255,255,255,  0,  0,  3,  3,  3,  0,  0,  3,  0,  0,255,255,255},
{255,255,255,255,255,  0,  0,  0,  0,  0,  0,255,255,255,255,255}
};

// game state
static int16_t car_x = SCREEN_WIDTH / 2;
static int16_t car_y = SCREEN_HEIGHT - 40;
static int16_t car_speed = 5;

static Bullet bullets[MAX_BULLETS];
static Fruit fruits[MAX_FRUITS];
static uint32_t score = 0;

// init fruits
static void InitFruits(void) {
    for (int i = 0; i < MAX_FRUITS; i++) {
        fruits[i].x = rand() % (SCREEN_WIDTH - 20) + 10;
        fruits[i].y = -(rand() % 200);
        fruits[i].old_x = fruits[i].x;
        fruits[i].old_y = fruits[i].y;
        fruits[i].active = 1;
        fruits[i].type = rand() % 3;
    }
}

// fire bullet
static void FireBullet(void) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
            bullets[i].x = car_x;
            bullets[i].y = car_y - 10;
            bullets[i].active = 1;

            buzzer_tone(&buzzer_cfg, 1500, 20);
            HAL_Delay(10);
            buzzer_off(&buzzer_cfg);
            break;
        }
    }
}

// collision check
static uint8_t CheckCollision(Bullet *b, Fruit *f) {
    if (!b->active || !f->active) return 0;

    if (b->x >= f->x - 8 && b->x <= f->x + 8 &&
        b->y >= f->y - 8 && b->y <= f->y + 8) {
        return 1;
    }
    return 0;
}

// main game loop
MenuState Game2_Run(void) {

    car_x = SCREEN_WIDTH / 2;
    score = 0;

    InitFruits();

    for (int i = 0; i < MAX_BULLETS; i++)
        bullets[i].active = 0;

    LCD_Set_Palette(PALETTE_CUSTOM);

    MenuState exit_state = MENU_STATE_HOME;

    while (1) {

        uint32_t frame_start = HAL_GetTick();

        Input_Read();
        Joystick_Read(&joystick_cfg, &joystick_data);
                // exit
        if (current_input.btn2_pressed) {
            exit_state = MENU_STATE_HOME;
            break;
        }

        // movement
        if (joystick_data.direction == W)
            car_x -= car_speed;

        if (joystick_data.direction == E)
            car_x += car_speed;

        if (car_x < 10) car_x = 10;
        if (car_x > SCREEN_WIDTH - 10) car_x = SCREEN_WIDTH - 10;

        // shoot
        if (current_input.btn3_pressed) {
            FireBullet();
        }

        // update bullets
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                bullets[i].y -= 8;
                if (bullets[i].y < 0)
                    bullets[i].active = 0;
            }
        }

        // update fruits
        for (int i = 0; i < MAX_FRUITS; i++) {
            if (fruits[i].active) {
                fruits[i].old_x = fruits[i].x;
                fruits[i].old_y = fruits[i].y;

                fruits[i].y += 3;

                if (fruits[i].y > SCREEN_HEIGHT) {
                    fruits[i].y = -20;
                    fruits[i].x = rand() % (SCREEN_WIDTH - 20) + 10;
                    fruits[i].type = rand() % 3;
                    fruits[i].old_x = fruits[i].x;
                    fruits[i].old_y = fruits[i].y;
                }
            }
        }

        // collisions
        for (int i = 0; i < MAX_BULLETS; i++) {
            for (int j = 0; j < MAX_FRUITS; j++) {
                if (CheckCollision(&bullets[i], &fruits[j])) {

                    bullets[i].active = 0;
                    fruits[j].active = 0;
                    score += 10;

                    buzzer_tone(&buzzer_cfg, 2000, 30);
                    HAL_Delay(20);
                    buzzer_off(&buzzer_cfg);

                    fruits[j].x = rand() % (SCREEN_WIDTH - 20) + 10;
                    fruits[j].y = -(rand() % 200);
                    fruits[j].old_x = fruits[j].x;
                    fruits[j].old_y = fruits[j].y;
                    fruits[j].active = 1;
                    fruits[j].type = rand() % 3;
                }
            }
        }
        


        // draw
        LCD_Fill_Buffer(0);

        LCD_printString("A", car_x - 4, car_y, 15, 3);

        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                LCD_printString("|", bullets[i].x, bullets[i].y, 15, 2);
            }
        }

        char score_str[32];
        sprintf(score_str, "Score: %lu", (unsigned long)score);
        LCD_printString(score_str, 10, 10, 15, 2);

        LCD_printString("Left/Right: Move", 10, SCREEN_HEIGHT - 40, 15, 1);
        LCD_printString("BTN3: Shoot", 10, SCREEN_HEIGHT - 25, 15, 1);
        LCD_printString("BTN2: Exit", 10, SCREEN_HEIGHT - 10, 15, 1);
        for (int i = 0; i < MAX_FRUITS; i++) {
            if (!fruits[i].active)
                continue;

            uint8_t* sprite = 0;

            switch (fruits[i].type) {
                case 0: sprite = (uint8_t*)appleSprite; break;
                case 1: sprite = (uint8_t*)orangeSprite; break;
                case 2: sprite = (uint8_t*)watermelonSprite; break;
            }

            LCD_Draw_Sprite(
                fruits[i].x - 8,
                fruits[i].y - 8,
                16,
                16,
                sprite
            );
        }

        LCD_Refresh(&cfg0);

        uint32_t frame_time = HAL_GetTick() - frame_start;
        if (frame_time < GAME2_FRAME_TIME_MS) {
            HAL_Delay(GAME2_FRAME_TIME_MS - frame_time);
        }
    }
    LCD_Set_Palette(PALETTE_DEFAULT);
    return exit_state;
}

