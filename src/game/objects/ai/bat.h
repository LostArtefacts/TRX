#pragma once

#include "global/types.h"

#include <stdint.h>

#define BAT_ATTACK_DAMAGE 2
#define BAT_TURN (20 * PHD_DEGREE) // = 3640
#define BAT_HITPOINTS 1
#define BAT_RADIUS (WALL_L / 10) // = 102
#define BAT_SMARTNESS 0x400

typedef enum {
    BAT_EMPTY = 0,
    BAT_STOP = 1,
    BAT_FLY = 2,
    BAT_ATTACK = 3,
    BAT_FALL = 4,
    BAT_DEATH = 5,
} BAT_ANIM;

extern BITE_INFO g_BatBite;

void Bat_Setup(OBJECT_INFO *obj);
void Bat_Initialise(int16_t item_num);
void Bat_Control(int16_t item_num);
