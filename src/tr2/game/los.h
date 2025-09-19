#pragma once

#include <libtrx/game/los.h>
#include <libtrx/game/rooms/types.h>

int32_t LOS_CheckX(const GAME_VECTOR *start, GAME_VECTOR *target);
int32_t LOS_CheckZ(const GAME_VECTOR *start, GAME_VECTOR *target);
int32_t LOS_ClipTarget(
    const GAME_VECTOR *start, GAME_VECTOR *target, const SECTOR *sector);
