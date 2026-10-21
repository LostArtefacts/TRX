#pragma once

#include <trx/game/collision/types.h>
#include <trx/game/rooms/types.h>

#define COLLIDE_MAX_COLLIDED 32

// The items and static meshes that one item touches.
typedef struct {
    ITEM *items[COLLIDE_MAX_COLLIDED];
    int32_t item_count;
    const STATIC_MESH *statics[COLLIDE_MAX_COLLIDED];
    int32_t static_count;
} COLLIDED_OBJECTS;

int32_t Collide_GetSpheres(const ITEM *item, SPHERE *spheres, bool world_space);

int32_t Collide_TestCollision(ITEM *item, const ITEM *lara_item);

void Collide_GetJointAbsPosition(
    const ITEM *item, XYZ_32 *out_vec, int32_t joint);

void Collide_GetCollisionInfo(
    COLL_INFO *coll, XYZ_32 pos, int16_t room_num, int32_t obj_height);

bool Collide_CollideStaticObjects(
    COLL_INFO *coll, XYZ_32 pos, int16_t room_num, int32_t height);
bool Collide_TestBoundsCollide(
    const COLL_ITEM *src_item, const COLL_ITEM *dst_item, int32_t radius);

void Collide_DoProperDetection(ITEM *item, XYZ_32 old_pos);

void Collide_ShiftItem(ITEM *item, COLL_INFO *coll);

// Finds what an item touches, in its room and the rooms its portals lead to,
// as TR4 finds it: each item and static mesh whose box comes within radius
// of the item's position, with 128 units to spare. Items that are not drawn
// or do not collide are left out, as are invisible items where
// skip_invisible is set, and Lara unless include_lara is set.
void Collide_GetCollidedObjects(
    const ITEM *item, int32_t radius, bool skip_invisible, bool include_lara,
    COLLIDED_OBJECTS *out);
