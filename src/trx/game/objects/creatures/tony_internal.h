#pragma once

#include <trx/core/math/types.h>
#include <trx/game/items/types.h>

// clang-format off
#define TONY_FIRE_BALL_DAMAGE 200
// clang-format on

void TonyBoss_TriggerFireBall(
    ITEM *item, int32_t type, const XYZ_32 *pos, int16_t room_num,
    int16_t angle, int32_t speed);
