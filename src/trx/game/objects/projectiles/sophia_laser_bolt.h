#pragma once

#include <trx/core/math/types.h>
#include <trx/game/items/types.h>

// clang-format off
#define SOPHIA_LASER_BOLT_DAMAGE            30
#define SOPHIA_BIG_LASER_BOLT_DAMAGE        542
#define SOPHIA_LASER_BOLT_SPLASH_DAMAGE     16
#define SOPHIA_BIG_LASER_BOLT_SPLASH_DAMAGE 64
// clang-format on

void Sophia_TriggerLaserBolt(
    XYZ_32 pos, const ITEM *item, int32_t type, int16_t angle);
