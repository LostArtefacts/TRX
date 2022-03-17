#pragma once

#include "global/const.h"
#include "global/types.h"

#include <stdint.h>

#define PEOPLE_POSE_CHANCE 0x60 // = 96
#define PEOPLE_SHOOT_RANGE SQUARE(WALL_L * 7) // = 51380224
#define PEOPLE_SHOT_DAMAGE 50
#define PEOPLE_WALK_TURN (PHD_DEGREE * 3) // = 546
#define PEOPLE_RUN_TURN (PHD_DEGREE * 6) // = 1092
#define PEOPLE_WALK_RANGE SQUARE(WALL_L * 3) // = 9437184
#define PEOPLE_MISS_CHANCE 0x2000

bool Targetable(ITEM_INFO *item, AI_INFO *info);
int32_t ShotLara(
    ITEM_INFO *item, int32_t distance, BITE_INFO *gun, int16_t extra_rotation);
