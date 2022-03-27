#pragma once

#include "global/types.h"

#include <stdint.h>

#define TORSO_DIE_ANIM 13
#define TORSO_PART_DAMAGE 250
#define TORSO_ATTACK_DAMAGE 500
#define TORSO_TOUCH_DAMAGE 5
#define TORSO_NEED_TURN (PHD_DEGREE * 45) // = 8190
#define TORSO_TURN (PHD_DEGREE * 3) // = 546
#define TORSO_ATTACK_RANGE SQUARE(2600) // = 6760000
#define TORSO_CLOSE_RANGE SQUARE(2250) // = 5062500
#define TORSO_TLEFT 0x7FF0
#define TORSO_TRIGHT 0x3FF8000
#define TORSO_TOUCH (TORSO_TLEFT | TORSO_TRIGHT)
#define TORSO_HITPOINTS 500
#define TORSO_RADIUS (WALL_L / 3) // = 341
#define TORSO_SMARTNESS 0x7FFF

typedef enum {
    TORSO_EMPTY = 0,
    TORSO_STOP = 1,
    TORSO_TURN_L = 2,
    TORSO_TURN_R = 3,
    TORSO_ATTACK1 = 4,
    TORSO_ATTACK2 = 5,
    TORSO_ATTACK3 = 6,
    TORSO_FORWARD = 7,
    TORSO_SET = 8,
    TORSO_FALL = 9,
    TORSO_DEATH = 10,
    TORSO_KILL = 11,
} TORSO_ANIM;

void Torso_Setup(OBJECT_INFO *obj);
void Torso_Control(int16_t item_num);
