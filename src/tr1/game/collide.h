#pragma once

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

void Collide_GetCollisionInfo(
    COLL_INFO *coll, int32_t xpos, int32_t ypos, int32_t zpos, int16_t room_num,
    int32_t objheight);

bool Collide_CollideStaticObjects(
    COLL_INFO *coll, int32_t x, int32_t y, int32_t z, int16_t room_num,
    int32_t height);

int32_t Collide_GetSpheres(ITEM *item, SPHERE *slist, int32_t world_space);

int32_t Collide_TestCollision(ITEM *item, ITEM *lara_item);

void Collide_GetJointAbsPosition(ITEM *item, XYZ_32 *vec, int32_t joint);
