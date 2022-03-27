#pragma once

#include "global/types.h"

#include <stdint.h>

#define TREX_ATTACK_RANGE SQUARE(WALL_L * 4) // = 16777216
#define TREX_BITE_DAMAGE 10000
#define TREX_BITE_RANGE SQUARE(1500) // = 2250000
#define TREX_ROAR_CHANCE 512
#define TREX_RUN_RANGE SQUARE(WALL_L * 5) // = 26214400
#define TREX_RUN_TURN (4 * PHD_DEGREE) // = 728
#define TREX_TOUCH 0x3000
#define TREX_TOUCH_DAMAGE 1
#define TREX_TRAMPLE_DAMAGE 10
#define TREX_WALK_TURN (2 * PHD_DEGREE) // = 364
#define TREX_HITPOINTS 100
#define TREX_RADIUS (WALL_L / 3) // = 341
#define TREX_SMARTNESS 0x7FFF

typedef enum {
    TREX_EMPTY = 0,
    TREX_STOP = 1,
    TREX_WALK = 2,
    TREX_RUN = 3,
    TREX_ATTACK1 = 4,
    TREX_DEATH = 5,
    TREX_ROAR = 6,
    TREX_ATTACK2 = 7,
    TREX_KILL = 8,
} TREX_ANIM;

void TRex_Setup(OBJECT_INFO *obj);
void TRex_Control(int16_t item_num);
void TRex_LaraDeath(ITEM_INFO *item);
