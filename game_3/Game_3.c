#include "Game_3.h"
#include "InputHandler.h"
#include "Menu.h"
#include "LCD.h"
#include "PWM.h"
#include "Buzzer.h"
#include "stm32l4xx_hal.h"
#include <stdio.h>


extern ST7789V2_cfg_t cfg0;
extern PWM_cfg_t pwm_cfg;      // LED PWM control
extern Buzzer_cfg_t buzzer_cfg; // Buzzer control
extern Joystick_cfg_t joystick_cfg;
extern Joystick_t joystick_data;


//game constants
#define screenWidth 240
#define screenHeight 240
#define maxTargets 4
#define targetRadius 10
#define crosshairSize 8
#define baseSpeed 120.0f
#define fleeForce 200.0f
#define comboTimeMS 1500
#define highScoreReg 0


//sprite
static const uint8_t targetSprite[16][16] = {
{255,255,255,2,2,2,2,2,2,2,2,2,255,255,255,255},
{255,255,2,3,3,3,3,3,3,3,3,3,2,255,255,255},
{255,2,3,3,3,3,3,3,3,3,3,3,3,2,255,255},
{2,3,3,3,1,1,3,3,3,3,1,1,3,3,2,255},
{2,3,3,1,1,1,3,3,3,3,1,1,1,3,2,255},
{2,3,3,1,1,1,3,3,3,3,1,1,1,3,2,255},
{2,3,3,3,1,1,3,3,3,3,1,1,3,3,2,255},
{2,3,3,3,3,3,3,3,3,3,3,3,3,3,2,255},
{2,3,3,3,3,3,3,3,3,3,3,3,3,3,2,255},
{255,2,3,3,3,3,3,3,3,3,3,3,3,2,255,255},
{255,255,2,3,3,3,3,3,3,3,3,3,2,255,255,255},
{255,255,255,2,3,3,3,3,3,3,3,2,255,255,255,255},
{255,255,255,255,2,3,3,3,3,3,2,255,255,255,255,255},
{255,255,255,255,255,2,2,2,2,2,255,255,255,255,255,255},
{255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255},
{255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255}
};

static const uint8_t crosshairSprite[16][16] = {
{255,255,255,255,255,255,255,2,2,255,255,255,255,255,255,255},
{255,255,255,255,255,255,255,2,2,255,255,255,255,255,255,255},
{255,255,255,255,255,255,255,2,2,255,255,255,255,255,255,255},
{255,255,255,255,255,255,255,2,2,255,255,255,255,255,255,255},
{255,255,255,255,255,255,255,2,2,255,255,255,255,255,255,255},
{255,255,255,255,255,255,255,2,2,255,255,255,255,255,255,255},
{255,255,255,255,255,255,255,2,2,255,255,255,255,255,255,255},
{2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2},
{2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2},
{255,255,255,255,255,255,255,2,2,255,255,255,255,255,255,255},
{255,255,255,255,255,255,255,2,2,255,255,255,255,255,255,255},
{255,255,255,255,255,255,255,2,2,255,255,255,255,255,255,255},
{255,255,255,255,255,255,255,2,2,255,255,255,255,255,255,255},
{255,255,255,255,255,255,255,2,2,255,255,255,255,255,255,255},
{255,255,255,255,255,255,255,2,2,255,255,255,255,255,255,255},
{255,255,255,255,255,255,255,2,2,255,255,255,255,255,255,255}
};


// game structures
typedef enum{
stateMenu,
statePlaying,
statePaused,
StateGameOver
} GameState;

typedef struct{
float x;
float y;
} Vector2D;

typedef struct{
Vector2D pos;
Vector2D vel;
uint8_t active;
uint32_t hitFlashEnd;
} Target_t;

typedef struct{
GameState state;
Target_t targets[maxTargets];
uint8_t targetCount;
Vector2D crosshair;
uint16_t score;
uint8_t score;
uint32_t lastHitTime;
float gameSpeed;
float sensitivity;
uint16_t highScore;
uint32_t ledFlashEnd;
} TargetGameEngine_t;

//global initialisation
static TargetGameEngine_t game;

//helper functions

static uint32_t randomGen(uint32_t max){
    static uint32_t seed = 12345;
    seed = seed * 1103515245 + 12345;
    return (seed >> 16)%max;
}

static void startLedFlash(uint32_t durationMS){
    game.ledFlashEnd = HAL_GetTick()+durationMS;
    PWM_SetDuty(&pwm_cfg,80)
    }

static void updateLedFlash(void){
    if(game.ledFlashEnd && HAL_GetTick() >= game.ledFlashEnd){
    PWM_SetDuty(&pwm_cfg,0);
    game.ledFlashEnd=0;
    }

}

static void playHitSound(uint8_t combo){
    uint16_t frequency = 1000 + (combo*100);
    if (freq>3000){
        frequency=3000;
    }    
    buzzer_tone(&buzzer_cfg,frequency,30);
    HAL_Delay(30);
    buzzer_off(&buzzer_cfg);
    
}

static void playMissSound(void){
    buzzer_tone(&buzzer_cfg,300,30);
    HAL_Delay(50);
    buzzer_off(&buzzer_cfg);
}

static void playGameOverSound(void){
    buzzer_tone(&buzzer_cfg,150,30);
    HAL_Delay(300);
    buzzer_off(&buzzer_cfg);
    buzzer_tone(&buzzer_cfg,100,30);
    HAL_Delay(500);
    buzzeer_off(&buzzer_cfg);
    
}

// Frame rate for this game (in milliseconds) - fastest game
#define GAME3_FRAME_TIME_MS 16  // ~60 FPS (faster than others!)

MenuState Game3_Run(void) {
    // Initialize game state
    animation_counter = 0;
    moving_x = 50;
    moving_y = 50;
    dx = 2;
    dy = 2;
    
    // Play a brief startup sound
    buzzer_tone(&buzzer_cfg, 1500, 30);  // 1.5kHz at 30% volume
    HAL_Delay(50);  // Brief beep duration
    buzzer_off(&buzzer_cfg);  // Stop the buzzer
    
    MenuState exit_state = MENU_STATE_HOME;  // Default: return to menu
    
    // Game's own loop - runs until exit condition
    while (1) {
        uint32_t frame_start = HAL_GetTick();
        
        // Read input
        Input_Read();
        
        // Check if button was pressed to return to menu
        if (current_input.btn3_pressed) {
            PWM_SetDuty(&pwm_cfg, 50);  // Reset LED
            exit_state = MENU_STATE_HOME;
            break;  // Exit game loop
        }
        
        // UPDATE: Game logic
        animation_counter++;
        
        // Simple animation: move object diagonally
        moving_x += dx;
        moving_y += dy;
        if (moving_x >= 200 || moving_x <= 0) {
            dx *= -1;
        }
        if (moving_y >= 200 || moving_y <= 0) {
            dy *= -1;
        }
        
        // Example: Vary LED brightness based on distance from origin
        uint8_t brightness = 30 + ((moving_x + moving_y) * 40) / 400;
        if (brightness > 100) brightness = 100;
        PWM_SetDuty(&pwm_cfg, brightness);
        
        // RENDER: Draw to LCD
        LCD_Fill_Buffer(0);
        
        // Title
        LCD_printString("GAME 3", 60, 10, 1, 3);
        
        // Simple animated object (moving box, diagonal)
        LCD_printString("[o]", 20 + moving_x, 50 + moving_y, 1, 3);
        
        // Display counter
        char counter[32];
        sprintf(counter, "Frame: %lu", animation_counter);
        LCD_printString(counter, 50, 140, 1, 2);
        
        // Show frame rate
        LCD_printString("Fast Demo", 20, 180, 1, 1);
        LCD_printString("60 FPS", 20, 195, 1, 1);
        
        // Instructions
        LCD_printString("Press BT3 to", 40, 220, 1, 1);
        LCD_printString("Return to Menu", 40, 235, 1, 1);
        
        LCD_Refresh(&cfg0);
        
        // Frame timing - wait for remainder of frame time
        uint32_t frame_time = HAL_GetTick() - frame_start;
        if (frame_time < GAME3_FRAME_TIME_MS) {
            HAL_Delay(GAME3_FRAME_TIME_MS - frame_time);
        }
    }
    
    return exit_state;  // Tell main where to go next
}
