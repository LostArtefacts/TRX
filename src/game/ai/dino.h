#pragma once

#include "global/types.h"

#include <stdint.h>

#define DINO_ATTACK_RANGE SQUARE(WALL_L * 4) // = 16777216
#define DINO_BITE_DAMAGE 10000
#define DINO_BITE_RANGE SQUARE(1500) // = 2250000
#define DINO_ROAR_CHANCE 512
#define DINO_RUN_RANGE SQUARE(WALL_L * 5) // = 26214400
#define DINO_RUN_TURN (4 * PHD_DEGREE) // = 728
#define DINO_TOUCH 0x3000
#define DINO_TOUCH_DAMAGE 1
#define DINO_TRAMPLE_DAMAGE 10
#define DINO_WALK_TURN (2 * PHD_DEGREE) // = 364
#define DINO_HITPOINTS 100
#define DINO_RADIUS (WALL_L / 3) // = 341
#define DINO_SMARTNESS 0x7FFF

typedef enum {
    DINO_EMPTY = 0,
    DINO_STOP = 1,
    DINO_WALK = 2,
    DINO_RUN = 3,
    DINO_ATTACK1 = 4,
    DINO_DEATH = 5,
    DINO_ROAR = 6,
    DINO_ATTACK2 = 7,
    DINO_KILL = 8,
} DINO_ANIM;

void SetupDino(OBJECT_INFO *obj);
void DinoControl(int16_t item_num);
void LaraDinoDeath(ITEM_INFO *item);
