#pragma once

#include <trx/game/rooms/types.h>
#include <trx/game/types.h>

bool LOS_Check(
    const GAME_VECTOR *start, GAME_VECTOR *target, bool use_detailed_clipping);
bool LOS_CheckItemIntersectSegment(
    const GAME_VECTOR *start, GAME_VECTOR *target, const ITEM *item);
// Finds the nearest static that a shot along the segment shatters, up to the
// first wall. Returns nullptr when there is none. On success, writes the
// position where the segment enters the static.
const STATIC_MESH *LOS_FindShatterableStatic(
    const GAME_VECTOR *start, const GAME_VECTOR *target,
    GAME_VECTOR *out_hit_pos);
int32_t LOS_CheckSmashable(
    GAME_VECTOR start, GAME_VECTOR target, XYZ_32 *out_hit_pos);
