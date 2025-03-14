#pragma once

#include <libtrx/game/collision.h>

int32_t Collide_TestCollision(ITEM *item, const ITEM *lara_item);

void Collide_GetJointAbsPosition(
    const ITEM *item, XYZ_32 *out_vec, int32_t joint);
