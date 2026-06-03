#pragma once

#include <trx/core/math/types.h>
#include <trx/game/items/types.h>

// clang-format off
#define SOPHIA_KNOCKBACK_DAMAGE             200
#define SOPHIA_LASER_BOLT_DAMAGE            30
#define SOPHIA_BIG_LASER_BOLT_DAMAGE        542
#define SOPHIA_LASER_BOLT_SPLASH_DAMAGE     16
#define SOPHIA_BIG_LASER_BOLT_SPLASH_DAMAGE 64
#define SOPHIA_PLASMA_BALL_DAMAGE           25
// clang-format on

void Sophia_TriggerPlasmaBall(
    int32_t type, XYZ_32 pos, int16_t room_num, int16_t angle);
void Sophia_TriggerLaserBolt(
    XYZ_32 pos, const ITEM *item, int32_t type, int16_t angle);
