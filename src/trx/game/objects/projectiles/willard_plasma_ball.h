#pragma once

#include <trx/core/math/types.h>

// clang-format off
#define WILLARD_PLASMA_BALL_DAMAGE 10000
// clang-format on

void Willard_TriggerPlasmaBall(
    XYZ_32 pos, int16_t room_num, int16_t angle, int16_t type);
