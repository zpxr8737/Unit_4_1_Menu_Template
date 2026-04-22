#ifndef GAME_2_H
#define GAME_2_H

#include "Menu.h"
#include "stm32l4xx_hal.h"

#define GAME2_FRAME_TIME_MS 50
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320
#define MAX_FRUITS 5
#define MAX_BULLETS 3

typedef struct {
    int16_t x, y;
    uint8_t active;
} Bullet;

typedef struct {
    int16_t x, y;
    int16_t old_x, old_y;
    uint8_t active;
    uint8_t type;
} Fruit;

MenuState Game2_Run(void);

#endif
