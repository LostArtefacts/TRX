#pragma once

#include <libtrx/game/collision.h>
#include <libtrx/game/items/types.h>

int32_t Collide_GetSpheres(ITEM *item, SPHERE *slist, int32_t world_space);

int32_t Collide_TestCollision(ITEM *item, ITEM *lara_item);

void Collide_GetJointAbsPosition(ITEM *item, XYZ_32 *vec, int32_t joint);
