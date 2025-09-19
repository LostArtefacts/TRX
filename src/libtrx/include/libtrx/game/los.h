#pragma once

#include "./types.h"

bool LOS_Check(const GAME_VECTOR *start, GAME_VECTOR *target);
int32_t LOS_CheckSmashable(XYZ_32 start, XYZ_32 target, XYZ_32 *out_hit_pos);
