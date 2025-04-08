#pragma once

#include "global/const.h"
#include "global/types.h"

#include <libtrx/game/creature.h>
#include <libtrx/utils.h>

bool Creature_ShootAtLara(
    ITEM *item, int32_t distance, BITE *gun, int16_t extra_rotation,
    int16_t damage);
bool Creature_IsBoss(int16_t item_num);
