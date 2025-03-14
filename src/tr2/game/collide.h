#pragma once

#include <libtrx/game/collision.h>
#include <libtrx/game/items/types.h>

int32_t Collide_TestCollision(ITEM *item, const ITEM *lara_item);

int32_t Collide_GetSpheres(const ITEM *item, SPHERE *spheres, bool world_space);

void Collide_GetJointAbsPosition(
    const ITEM *item, XYZ_32 *out_vec, int32_t joint);
