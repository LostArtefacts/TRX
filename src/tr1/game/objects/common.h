#pragma once

// Generic collision and draw routines reused between various objects

#include "global/types.h"

int16_t Object_FindReceptacle(GAME_OBJECT_ID obj_id);
void Object_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
void Object_CollisionTrap(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);

void Object_DrawInterpolatedObject(
    const OBJECT *obj, uint32_t meshes, const int16_t *extra_rotation,
    const ANIM_FRAME *frame1, const ANIM_FRAME *frame2, int32_t frac,
    int32_t rate);
void Object_DrawDummyItem(const ITEM *item);
void Object_DrawSpriteItem(const ITEM *item);
void Object_DrawPickupItem(const ITEM *item);
void Object_DrawAnimatingItem(const ITEM *item);
void Object_DrawUnclippedItem(const ITEM *item);
void Object_SetMeshReflective(
    GAME_OBJECT_ID obj_id, int32_t mesh_idx, bool enabled);
void Object_SetReflective(GAME_OBJECT_ID obj_id, bool enabled);
