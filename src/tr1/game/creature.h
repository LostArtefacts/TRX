#pragma once

#include "global/const.h"
#include "global/types.h"

#include <libtrx/game/creature.h>
#include <libtrx/utils.h>

#define CREATURE_SHOOT_RANGE SQUARE(WALL_L * 7) // = 51380224
#define CREATURE_MISS_CHANCE 0x2000

bool Creature_CheckBaddieOverlap(int16_t item_num);
bool Creature_CanTargetEnemy(ITEM *item, AI_INFO *info);
bool Creature_ShootAtLara(
    ITEM *item, int32_t distance, BITE *gun, int16_t extra_rotation,
    int16_t damage);
bool Creature_IsBoss(int16_t item_num);
