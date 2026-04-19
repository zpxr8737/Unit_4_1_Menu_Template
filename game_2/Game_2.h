#ifndef GAME_2_H
#define GAME_2_H

#include "Menu.h"
#include "stm32l4xx_hal.h"

/**
 * @file Game_2.h
 * @brief First-Person Car Shooting Game for STM32
 * 
 * This header declares the main game function for a first-person car shooting game.
 * The player moves a car along a track using buttons, shoots targets (fruit sprites),
 * and receives sound feedback via a buzzer for shooting and hitting targets.
 * The game is designed to be RAM-efficient, drawing only updated objects directly to the LCD.
 */

// Frame rate (milliseconds per frame)
#define GAME2_FRAME_TIME_MS 50  // ~20 FPS

// Track / screen dimensions
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

// Maximum objects
#define MAX_FRUITS 5
#define MAX_BULLETS 3

// Data structures for bullets and fruits
typedef struct {
    int16_t x, y;
    uint8_t active;
} Bullet;

typedef struct {
    int16_t x, y;
    int16_t old_x, old_y;  // previous position for erasing
    uint8_t active;
    uint8_t type;  // 0 = apple, 1 = banana, 2 = cherry
} Fruit;

/**
 * @brief Run the First-Person Car Shooting Game
 * 
 * This function contains the main game loop. It handles:
 * - Reading button input for car movement and shooting
 * - Updating bullet and fruit positions
 * - Checking collisions between bullets and fruits
 * - Drawing the car, bullets, and fruits directly to the LCD
 * - Playing sound effects through the buzzer
 * - Displaying score and instructions
 * 
 * The function runs until the player presses the exit button (BTN3) and returns
 * the menu state to switch back to the main menu.
 * 
 * @return MenuState - Typically MENU_STATE_HOME to return to main menu
 */
MenuState Game2_Run(void);

#endif // GAME_2_H 