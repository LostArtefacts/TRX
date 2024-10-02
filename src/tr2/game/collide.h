#pragma once

#include "global/types.h"

void __cdecl Collide_GetCollisionInfo(
    COLL_INFO *coll, int32_t x_pos, int32_t y_pos, int32_t z_pos,
    int16_t room_num, int32_t obj_height);

int32_t __cdecl Collide_CollideStaticObjects(
    COLL_INFO *coll, int32_t x, int32_t y, int32_t z, int16_t room_num,
    int32_t height);
