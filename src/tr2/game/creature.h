#pragma once

#include "global/types.h"

#include <libtrx/game/creature.h>

bool Creature_IsAlly(const ITEM *item);
int32_t Creature_ShootAtLara(
    ITEM *item, const AI_INFO *info, const BITE *gun, int16_t extra_rotation,
    int32_t damage);
