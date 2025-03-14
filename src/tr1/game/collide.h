#pragma once

#include <libtrx/game/collision.h>

int32_t Collide_TestCollision(ITEM *item, const ITEM *lara_item);

void Collide_GetJointAbsPosition(ITEM *item, XYZ_32 *vec, int32_t joint);
