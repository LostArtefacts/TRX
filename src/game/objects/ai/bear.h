#pragma once

#include "global/types.h"

#include <stdint.h>

#define BEAR_CHARGE_DAMAGE 3
#define BEAR_SLAM_DAMAGE 200
#define BEAR_ATTACK_DAMAGE 200
#define BEAR_PAT_DAMAGE 400
#define BEAR_TOUCH 0x2406C
#define BEAR_ROAR_CHANCE 80
#define BEAR_REAR_CHANCE 768
#define BEAR_DROP_CHANCE 1536
#define BEAR_REAR_RANGE SQUARE(WALL_L * 2) // = 4194304
#define BEAR_ATTACK_RANGE SQUARE(WALL_L) // = 1048576
#define BEAR_PAT_RANGE SQUARE(600) // = 360000
#define BEAR_RUN_TURN (5 * PHD_DEGREE) // = 910
#define BEAR_WALK_TURN (2 * PHD_DEGREE) // = 364
#define BEAR_EAT_RANGE SQUARE(WALL_L * 3 / 4) // = 589824
#define BEAR_HITPOINTS 20
#define BEAR_RADIUS (WALL_L / 3) // = 341
#define BEAR_SMARTNESS 0x4000

typedef enum {
    BEAR_STROLL = 0,
    BEAR_STOP = 1,
    BEAR_WALK = 2,
    BEAR_RUN = 3,
    BEAR_REAR = 4,
    BEAR_ROAR = 5,
    BEAR_ATTACK1 = 6,
    BEAR_ATTACK2 = 7,
    BEAR_EAT = 8,
    BEAR_DEATH = 9,
} BEAR_ANIM;

extern BITE_INFO g_BearHeadBite;

void Bear_Setup(OBJECT_INFO *obj);
void Bear_Control(int16_t item_num);
