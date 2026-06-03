#pragma once

#include <trx/game/items.h>

// clang-format off
#define CLAW_MUTANT_PLASMA_BALL_DAMAGE 200
// clang-format on

void ClawMutant_TriggerPlasmaBall(
    const ITEM *item, const XYZ_32 *pos, int16_t room_num);
