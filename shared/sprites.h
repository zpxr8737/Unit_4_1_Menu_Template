#ifndef SPRITES_H
#define SPRITES_H

#include <stdint.h>

// we can store sprites as 2D arrays, we use uint8_t (8bit int) to save space.
// 255 means transparent pixel. The numbers refer to the current colour palette
//  0=LCD_COLOUR_0/black, 1=LCD_COLOUR_1/white, etc. if we are using the default palette
 
extern const uint8_t CAR_STRAIGHT[64][64];

extern const uint8_t SKY[120][300];

extern const uint8_t appleSprite[16][16];

extern const uint8_t orangeSprite[16][16];

extern const uint8_t watermelonSprite[16][16];

extern const uint8_t LFS_SPLASHSCREEN[240][240];

extern const uint8_t CAR_LEFT[64][64];

extern const uint8_t CAR_RIGHT[64][64];

extern const uint8_t crosshairSprite[16][16];

#endif