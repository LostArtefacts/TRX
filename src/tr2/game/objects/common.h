#pragma once

#include "global/types.h"

#include <libtrx/game/objects/common.h>

void Object_DrawDummyItem(const ITEM *item);
void Object_DrawAnimatingItem(const ITEM *item);
void Object_DrawSpriteItem(const ITEM *item);

void Object_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);

void Object_Collision_Trap(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);

BOUNDS_16 Object_GetBoundingBox(
    const OBJECT *obj, const ANIM_FRAME *frame, uint32_t mesh_bits);

void Object_DrawMesh(int32_t mesh_idx, int32_t clip, bool interpolated);
