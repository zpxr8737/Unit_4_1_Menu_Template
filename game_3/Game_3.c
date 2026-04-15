#include "Game_3.h"
#include "InputHandler.h"
#include "Menu.h"
#include "LCD.h"
#include "PWM.h"
#include "Buzzer.h"
#include "Joystick.h"
#include "stm32l4xx_hal.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

extern ST7789V2_cfg_t cfg0;
extern PWM_cfg_t pwm_cfg;      // LED PWM control
extern Buzzer_cfg_t buzzer_cfg; // Buzzer control
extern Joystick_cfg_t joystick_cfg;
extern Joystick_t joystick_data;


//game constants
#define screenWidth 240
#define screenHeight 240
#define maxTargets 4
#define baseSpeed 120.0f
#define comboTimeMS 1500
#define gameFrameMS 16
#define longPressMS 500

//fruit types
typedef enum{
    fruitApple,
    fruitOrange,
    fruitWatermelon
} FruitType;


// game structures
typedef enum{
stateMenu,
statePlaying,
statePaused,
stateGameOver
} GameState;

typedef struct{
Vector2D pos;
Vector2D vel;
uint8_t active;
uint32_t hitFlashEnd;
uint8_t type;
uint8_t radius;
uint32_t fleeStartTime;
} Target_t;

typedef struct{
GameState state;
Target_t targets[maxTargets];
uint8_t targetCount;
Vector2D crosshair;
Vector2D crosshairTarget;
uint16_t score;
uint8_t lives;
uint8_t combo;
uint32_t lastHitTime;
float gameSpeed;
float sensitivity;
uint16_t highScore;
uint32_t ledFlashEnd;
} TargetGameEngine_t;

//sprites

static const uint8_t appleSprite[32][32] = {
{255,255,255,255,255,255,255,255,255,255,255,3,3,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255},
{255,255,255,255,255,255,255,255,255,255,3,3,3,3,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255},
{255,255,255,255,255,255,255,255,255,255,255,3,3,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255},

{255,255,255,255,255,255,255,255,255,2,2,2,2,2,2,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255},
{255,255,255,255,255,255,255,2,2,2,2,2,2,2,2,2,2,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255},
{255,255,255,255,255,255,2,2,2,2,2,2,2,2,2,2,2,2,255,255,255,255,255,255,255,255,255,255,255,255,255,255},

{255,255,255,255,255,2,2,2,2,2,2,2,2,2,2,2,2,2,2,255,255,255,255,255,255,255,255,255,255,255,255,255},
{255,255,255,255,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,255,255,255,255,255,255,255,255,255,255,255,255},

{255,255,255,255,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,255,255,255,255,255,255,255,255,255,255,255,255},
{255,255,255,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,255,255,255,255,255,255,255,255,255,255,255},

{255,255,255,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,255,255,255,255,255,255,255,255,255,255,255},
{255,255,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,255,255,255,255,255,255,255,255,255,255},

{255,255,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,255,255,255,255,255,255,255,255,255,255},
{255,255,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,255,255,255,255,255,255,255,255,255,255},

{255,255,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,255,255,255,255,255,255,255,255,255,255},
{255,255,255,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,255,255,255,255,255,255,255,255,255,255,255},

{255,255,255,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,255,255,255,255,255,255,255,255,255,255,255},
{255,255,255,255,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,255,255,255,255,255,255,255,255,255,255,255,255},

{255,255,255,255,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,255,255,255,255,255,255,255,255,255,255,255,255},
{255,255,255,255,255,2,2,2,2,2,2,2,2,2,2,2,2,2,2,255,255,255,255,255,255,255,255,255,255,255,255,255},

{255,255,255,255,255,255,2,2,2,2,2,2,2,2,2,2,2,2,255,255,255,255,255,255,255,255,255,255,255,255,255,255},
{255,255,255,255,255,255,255,2,2,2,2,2,2,2,2,2,2,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255},

{255,255,255,255,255,255,255,255,2,2,2,2,2,2,2,2,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255},
{255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255}
};

static const uint8_t orangeSprite[32][32] = {
{255,255,255,255,255,255,255,255,255,255,255,5,5,5,5,5,5,5,5,255,255,255,255,255,255,255,255,255,255,255,255,255},
{255,255,255,255,255,255,255,255,255,255,5,5,5,5,5,5,5,5,5,5,255,255,255,255,255,255,255,255,255,255,255,255},

{255,255,255,255,255,255,255,255,255,5,5,5,4,4,4,4,4,4,5,5,5,255,255,255,255,255,255,255,255,255,255,255},
{255,255,255,255,255,255,255,255,5,5,4,4,4,4,4,4,4,4,4,4,5,5,255,255,255,255,255,255,255,255,255,255},

{255,255,255,255,255,255,255,5,5,4,4,4,4,4,4,4,4,4,4,4,4,5,5,255,255,255,255,255,255,255,255,255},
{255,255,255,255,255,255,5,5,4,4,4,4,4,4,4,4,4,4,4,4,4,4,5,5,255,255,255,255,255,255,255,255},

{255,255,255,255,255,5,5,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,5,5,255,255,255,255,255,255,255},
{255,255,255,255,5,5,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,5,5,255,255,255,255,255,255},

{255,255,255,255,5,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,5,255,255,255,255,255,255},
{255,255,255,5,5,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,5,5,255,255,255,255,255},

{255,255,255,5,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,5,255,255,255,255,255},
{255,255,255,5,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,5,255,255,255,255,255},

{255,255,255,5,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,5,255,255,255,255,255},
{255,255,255,5,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,5,255,255,255,255,255},

{255,255,255,5,5,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,5,5,255,255,255,255,255},
{255,255,255,255,5,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,5,255,255,255,255,255,255},

{255,255,255,255,5,5,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,5,5,255,255,255,255,255,255},
{255,255,255,255,255,5,5,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,5,5,255,255,255,255,255,255,255},

{255,255,255,255,255,255,5,5,4,4,4,4,4,4,4,4,4,4,4,4,4,4,5,5,255,255,255,255,255,255,255,255},
{255,255,255,255,255,255,255,5,5,4,4,4,4,4,4,4,4,4,4,4,4,5,5,255,255,255,255,255,255,255,255,255},

{255,255,255,255,255,255,255,255,5,5,5,4,4,4,4,4,4,4,4,5,5,5,255,255,255,255,255,255,255,255,255,255},
{255,255,255,255,255,255,255,255,255,255,5,5,5,5,5,5,5,5,5,255,255,255,255,255,255,255,255,255,255,255,255,255}
};

static const uint8_t watermelonSprite[32][32] = {
{255,255,255,255,255,255,255,255,255,255,255,3,3,3,3,3,3,3,3,255,255,255,255,255,255,255,255,255,255,255,255,255},
{255,255,255,255,255,255,255,255,255,255,3,3,3,3,3,3,3,3,3,3,255,255,255,255,255,255,255,255,255,255,255,255},

{255,255,255,255,255,255,255,255,255,3,3,5,5,5,5,5,5,5,5,3,3,255,255,255,255,255,255,255,255,255,255,255},
{255,255,255,255,255,255,255,255,3,3,5,5,2,2,2,2,2,2,5,5,3,3,255,255,255,255,255,255,255,255,255,255},

{255,255,255,255,255,255,255,3,3,5,5,2,2,2,2,2,2,2,2,5,5,3,3,255,255,255,255,255,255,255,255,255},
{255,255,255,255,255,255,3,3,5,5,2,2,2,2,2,2,2,2,2,2,5,5,3,3,255,255,255,255,255,255,255,255},

{255,255,255,255,255,3,3,5,5,2,2,2,2,2,6,2,2,6,2,2,2,5,5,3,3,255,255,255,255,255,255,255},
{255,255,255,255,3,3,5,5,2,2,2,2,2,2,2,2,2,2,2,2,2,2,5,5,3,3,255,255,255,255,255,255},

{255,255,255,255,3,5,5,2,2,2,2,2,6,2,2,2,2,6,2,2,2,2,2,5,5,3,255,255,255,255,255,255},
{255,255,255,3,3,5,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,5,3,3,255,255,255,255,255},

{255,255,255,3,3,5,2,2,2,2,2,6,2,2,2,2,2,2,6,2,2,2,2,2,5,3,3,255,255,255,255,255},
{255,255,255,3,3,5,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,5,3,3,255,255,255,255,255},

{255,255,255,3,3,5,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,5,3,3,255,255,255,255,255},
{255,255,255,3,3,5,2,2,2,2,2,6,2,2,2,2,2,2,6,2,2,2,2,2,5,3,3,255,255,255,255,255},

{255,255,255,255,3,5,5,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,5,5,3,255,255,255,255,255,255},
{255,255,255,255,3,3,5,5,2,2,2,2,2,2,2,2,2,2,2,2,2,2,5,5,3,3,255,255,255,255,255,255},

{255,255,255,255,255,3,3,5,5,2,2,2,2,2,2,2,2,2,2,2,2,5,5,3,3,255,255,255,255,255,255,255},
{255,255,255,255,255,255,3,3,5,5,2,2,2,2,2,2,2,2,2,2,5,5,3,3,255,255,255,255,255,255,255,255},

{255,255,255,255,255,255,255,3,3,5,5,5,2,2,2,2,2,2,5,5,5,3,3,255,255,255,255,255,255,255,255,255},
{255,255,255,255,255,255,255,255,3,3,3,5,5,5,5,5,5,5,5,3,3,3,255,255,255,255,255,255,255,255,255,255}
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


//global initialisation
static TargetGameEngine_t game;

// fruit properties

static uint8_t getFruitRadius(uint8_t type){
    switch(type){
        case fruitApple: return 40;
        case fruitOrange: return 50;
        case fruitWatermelon: return 60;
        default: return 50;
    }
}

static uint16_t getFruitScore(uint8_t type){
    switch(type){
        case fruitApple: return 25;
        case fruitOrange: return 15;
        case fruitWatermelon: return 10;
        default: return 25;
    }
}

static float getFruitFleeForce(uint8_t type){
    switch(type){
        case fruitApple: return 250.0f;
        case fruitOrange: return 200.0f;
        case fruitWatermelon: return 150.0f;
        default: return 250.0f;
    }
    
}

static const uint8_t* getFruitSprite(uint8_t type){
    switch(type){
        case fruitApple: return (uint8_t*)appleSprite;
        case fruitOrange: return (uint8_t*)orangeSprite;
        case fruitWatermelon: return (uint8_t*)watermelonSprite;
        default: return (uint8_t*)appleSprite;
    }
    
}

static uint16_t getFruitHitSound(uint8_t type, uint8_t combo){
    uint16_t base;
    switch(type){
        case fruitApple: base = 900; break;
        case fruitOrange: base = 1100; break;
        case fruitWatermelon: base = 700; break;
        default: base = 1000; break;
    }
    uint16_t add = (combo>5 ? 5: combo) * 50;
    return base + add;
}

//helper functions

static uint32_t randomGen(uint32_t max){
    static uint32_t seed = 12345;
    seed = seed * 1103515245 + 12345;
    return (seed >> 16)%max;
}

static void startLedFlash(uint32_t durationMS){
    game.ledFlashEnd = HAL_GetTick()+durationMS;
    PWM_SetDuty(&pwm_cfg,80);
    }

static void updateLedFlash(void){
    if(game.ledFlashEnd && HAL_GetTick() >= game.ledFlashEnd){
    PWM_SetDuty(&pwm_cfg,0);
    game.ledFlashEnd=0;
    }

}

static void playHitSound(uint8_t type, uint8_t combo){
    uint16_t frequency = getFruitHitSound(type, combo);
    if (frequency > 3000) frequency = 3000;
    buzzer_tone(&buzzer_cfg, frequency, 30);
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
    buzzer_off(&buzzer_cfg);
    
}

// core gameplay

static void addTarget(void){
    for (int i=0; i<maxTargets; i++){
        if(!game.targets[i].active){
            game.targets[i].active = 1;
            game.targets[i].type = randomGen(3);
            game.targets[i].radius = getFruitRadius(game.targets[i].type);
            game.targets[i].pos.x = randomGen(screenWidth - 2*game.targets[i].radius)+game.targets[i].radius;
            game.targets[i].pos.y = randomGen(screenHeight - 2*game.targets[i].radius)+game.targets[i].radius;
            float angle = randomGen(360)*(M_PI/180.0f);
            float speed = baseSpeed * game.gameSpeed;
            game.targets[i].vel.x = cosf(angle)*speed;
            game.targets[i].vel.y=sinf(angle)*speed;
            game.targets[i].hitFlashEnd = 0;
            game.targets[i].fleeStartTime = HAL_GetTick() + 800;  // Start fleeing after 0.8 sec
            break;
        }
    }
    
    
}

static void resetGame(void){
    game.state=statePlaying;
    game.targetCount = 1;
    game.score= 0;
    game.lives=3;
    game.combo = 0;
    game.lastHitTime = 0;
    game.gameSpeed = 1.0f;
    game.sensitivity = 0.8f;
    game.crosshair.x = screenWidth/2;
    game.crosshair.y = screenHeight/2;
    game.crosshairTarget.x = screenWidth/2;
    game.crosshairTarget.y = screenHeight/2;
    game.ledFlashEnd=0;
    PWM_SetDuty(&pwm_cfg,0);
    for(int i =0; i< maxTargets;i++){
        game.targets[i].active=0;
    }
    addTarget();
}

static void updateHighScore(void){
    if(game.score>game.highScore){
        game.highScore = game.score;
    }
}
static void handleShot(Vector2D shotPos){
    if(game.state != statePlaying){
        return;
    }
    int hit = 0;
    for(int i =0; i<game.targetCount;i++){
        if(!game.targets[i].active){
            continue;
        }
        float dx = shotPos.x - game.targets[i].pos.x;
        float dy = shotPos.y - game.targets[i].pos.y;
        float distance = sqrtf(dx*dx+dy*dy);
        if(distance<game.targets[i].radius){
            hit=1;
            game.targets[i].active = 0;
            game.targets[i].hitFlashEnd = HAL_GetTick()+100;

            //combo
            uint32_t now = HAL_GetTick();
            if((now-game.lastHitTime)<comboTimeMS){
                game.combo++;
            }
            else{
                game.combo = 1;
            }

            game.lastHitTime = now;
            if(game.combo >5){
                game.combo = 5;
            }
            uint16_t addScore = getFruitScore(game.targets[i].type)*game.combo;
            game.score += addScore;
            playHitSound(game.targets[i].type,game.combo);
            startLedFlash(50);
            addTarget();
            break;
        }
    }
    if(!hit && game.lives>0){
        game.lives--;
        game.combo = 0;
        playMissSound();
        startLedFlash(100);
    }

}

static void updateGame(float deltaSec){
    if(game.state != statePlaying){
        return;
    }
    for(int i =0; i<game.targetCount;i++){
        if(!game.targets[i].active){
            continue;
        }
        //movements
        game.targets[i].pos.x += game.targets[i].vel.x *deltaSec;
        game.targets[i].pos.y += game.targets[i].vel.y *  deltaSec;

        //bounce
        if(game.targets[i].pos.x < game.targets[i].radius){
            game.targets[i].pos.x = game.targets[i].radius;
            game.targets[i].vel.x = -game.targets[i].vel.x;
        }
        if(game.targets[i].pos.x > screenWidth - game.targets[i].radius){
            game.targets[i].pos.x = screenWidth - game.targets[i].radius;
            game.targets[i].vel.x = -game.targets[i].vel.x;
            
        }
        if(game.targets[i].pos.y < game.targets[i].radius){
            game.targets[i].pos.y = game.targets[i].radius;
            game.targets[i].vel.y = -game.targets[i].vel.y;
            
        }
        if(game.targets[i].pos.y > screenHeight - game.targets[i].radius){
            game.targets[i].pos.y = screenHeight - game.targets[i].radius;
            game.targets[i].vel.y = -game.targets[i].vel.y;
            
        }
        //fleeing
        float dx = game.targets[i].pos.x - game.crosshair.x;
        float dy = game.targets[i].pos.y - game.crosshair.y;
        float distance = sqrtf(dx*dx + dy*dy);
        float fleeDistance = (game.targets[i].type == fruitWatermelon) ? 70.0f: 50.0f;
        if(distance<fleeDistance && distance > 0.1f && HAL_GetTick() >= game.targets[i].fleeStartTime){
            float awayx=dx/distance;
            float awayy = dy/distance;
            float force = getFruitFleeForce(game.targets[i].type);
            game.targets[i].vel.x +=awayx*force* deltaSec;
            game.targets[i].vel.y +=awayy*force*deltaSec;
            float maxSpeed = baseSpeed * game.gameSpeed * 2.0f;
            float speed = sqrtf(game.targets[i].vel.x*game.targets[i].vel.x+game.targets[i].vel.y*game.targets[i].vel.y);
            if(speed>maxSpeed){
                game.targets[i].vel.x =  game.targets[i].vel.x/speed*maxSpeed;
                game.targets[i].vel.y =  game.targets[i].vel.y/speed*maxSpeed;
            }
        }
    }

    //difficulty programming
    if(game.score>0){
        float newSpeed = 1.0f +(game.score/200.0f);
        if(newSpeed>3.0f){
            newSpeed=3.0f;
        }
        game.gameSpeed=newSpeed;
    }

//add more targets
    if(game.score >=150&& game.targetCount<2){
        game.targetCount = 2;
        addTarget();
    }
    if(game.score >= 300 && game.targetCount<3){
        game.targetCount=3;
        addTarget();
    }
    if(game.score >= 500 && game.targetCount<4){
        game.targetCount = 4;
        addTarget();
    }

    if(game.lives==0){
        game.state= stateGameOver;
        playGameOverSound();
        updateHighScore();
        startLedFlash(500);
        
    }


}

static void drawGame(void){
    LCD_Draw_Sprite((uint16_t)(game.crosshair.x-8), (uint16_t)(game.crosshair.y-8), 16, 16, (uint8_t*)crosshairSprite);

    //targets
    for(int i = 0; i<game.targetCount;i++){
        if(!game.targets[i].active){
            continue;
        }
        const uint8_t* sprite = getFruitSprite(game.targets[i].type);
        if(game.targets[i].hitFlashEnd && HAL_GetTick() < game.targets[i].hitFlashEnd){
            LCD_Draw_Sprite_Colour((uint16_t)(game.targets[i].pos.x-16),(uint16_t)(game.targets[i].pos.y - 16),32,32,sprite,1);
        }
        else{
            LCD_Draw_Sprite((uint16_t)(game.targets[i].pos.x-16), (uint16_t)(game.targets[i].pos.y-16), 32, 32, sprite);
        }
    }

    char buf[32];
    sprintf(buf,"Score: %d", game.score);
    LCD_printString(buf,10,10,1,2);
    sprintf(buf, "Lives: %d", game.lives);
    LCD_printString(buf, 140, 10, 1, 2);
    if(game.combo>1){
        sprintf(buf, "x%d COMBO!", game.combo);
        LCD_printString(buf, 80, 50, 2, 2);
    }

    // pause menu
    if(game.state==statePaused){
        LCD_Draw_Rect(20,30,200,180,0,1);
        LCD_printString("PAUSED", 75, 40, 1, 2);
        LCD_printString("Sensitivity:", 35, 80, 1, 1);
        sprintf(buf, "%.2f", game.sensitivity);
        LCD_printString(buf, 150, 80, 1, 1);
        LCD_printString("LEFT/RIGHT adjust", 35, 105, 1, 1);
        LCD_printString("BTN3: resume", 35, 125, 1, 1);
        LCD_printString("BTN2: menu", 35, 145, 1, 1);
    }
    //game over
    if(game.state==stateGameOver){
        LCD_printString("GAME OVER", 60, 30, 1, 2);
        sprintf(buf, "Score: %d", game.score);
        LCD_printString(buf, 70, 70, 1, 2);
        sprintf(buf, "High Score: %d", game.highScore);
        LCD_printString(buf, 50, 100, 1, 1);
        LCD_printString("BTN2: menu", 75, 150, 1, 2);
    }

}

//main code

MenuState Game3_Run(void){
    resetGame();

    buzzer_tone(&buzzer_cfg, 1500, 30);
    HAL_Delay(50);
    buzzer_off(&buzzer_cfg);

    uint32_t lastFrameTime = HAL_GetTick();

    while(1){
        uint32_t now= HAL_GetTick();
        float deltaSec = (now-lastFrameTime)/1000.0f;
        if(deltaSec > 0.05f) deltaSec = 0.05f;
        lastFrameTime = now;

        Input_Read();
        Joystick_Read(&joystick_cfg, &joystick_data);
        UserInput joyInput = Joystick_GetInput(&joystick_data);

        // BTN2 pauses/resumes
        if(current_input.btn2_pressed){
            if(game.state==statePlaying){
                game.state = statePaused;
            }
            else if(game.state==statePaused){
                game.state=statePlaying;
            }
            else if(game.state==stateGameOver){
                PWM_SetDuty(&pwm_cfg, 0);
                buzzer_off(&buzzer_cfg);
                return MENU_STATE_HOME;
            }
        }

        // BTN3 (joystick button) shoots or resumes from pause
        if(current_input.btn3_pressed){
            if(game.state==statePlaying){
                handleShot(game.crosshair);
            }
            else if(game.state==statePaused){
                game.state=statePlaying;
            }
            else if(game.state==stateGameOver){
                PWM_SetDuty(&pwm_cfg, 0);
                buzzer_off(&buzzer_cfg);
                return MENU_STATE_HOME;
            }
        }

        // update crosshair
        game.crosshairTarget.x = (joystick_data.coord_mapped.x) * (screenWidth/2)+(screenWidth/2);
        game.crosshairTarget.y = ((-joystick_data.coord_mapped.y)) * (screenHeight/2)+(screenHeight/2);
        
        //smooth crosshair movement with easing
        float easeSpeed = 0.15f * game.sensitivity;  // sensitivity now controls speed, not range
        game.crosshair.x += (game.crosshairTarget.x - game.crosshair.x) * easeSpeed;
        game.crosshair.y += (game.crosshairTarget.y - game.crosshair.y) * easeSpeed;
        if(game.crosshair.x<0){
            game.crosshair.x=0;
        }
        if(game.crosshair.x>screenWidth){
            game.crosshair.x = screenWidth;
        }
        if(game.crosshair.y<0){
            game.crosshair.y=0;
        }
        if(game.crosshair.y>screenHeight){
            game.crosshair.y = screenHeight;
        }

        if(game.state==statePaused){
            if(joyInput.direction == W){
                game.sensitivity -= 0.05f;
                if(game.sensitivity <0){
                    game.sensitivity =0;
                }
            }
            if(joyInput.direction == E){
                game.sensitivity +=0.05f;
                if(game.sensitivity>1){
                    game.sensitivity =1;
                }
            }
        }
        
            if(game.state == statePlaying){
                updateGame(deltaSec);
            }
            updateLedFlash();

            LCD_Fill_Buffer(0);
            drawGame();
            LCD_Refresh(&cfg0);

            uint32_t frameTime = HAL_GetTick() - now;
            if(frameTime<gameFrameMS){
                HAL_Delay(gameFrameMS-frameTime);
            }
    }
}
