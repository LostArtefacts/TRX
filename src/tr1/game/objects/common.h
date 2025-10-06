#pragma once

// Generic collision and draw routines reused between various objects

#include "global/types.h"

void Object_DrawSpriteItem(const ITEM *item);
void Object_DrawPickupItem(const ITEM *item);
void Object_DrawAnimatingItem(const ITEM *item);
void Object_SetMeshReflective(
    GAME_OBJECT_ID obj_id, int32_t mesh_idx, bool enabled);
