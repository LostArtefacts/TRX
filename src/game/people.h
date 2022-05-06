#pragma once

#include "global/const.h"
#include "global/types.h"

#define PEOPLE_SHOOT_RANGE SQUARE(WALL_L * 7) // = 51380224
#define PEOPLE_SHOT_DAMAGE 50
#define PEOPLE_MISS_CHANCE 0x2000

bool Targetable(ITEM_INFO *item, AI_INFO *info);
bool ShotLara(
    ITEM_INFO *item, int32_t distance, BITE_INFO *gun, int16_t extra_rotation);
