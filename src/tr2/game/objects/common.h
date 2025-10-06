#pragma once

#include "global/types.h"

#include <libtrx/game/objects/common.h>

void Object_DrawAnimatingItem(const ITEM *item);
void Object_DrawSpriteItem(const ITEM *item);

BOUNDS_16 Object_GetBoundingBox(
    const OBJECT *obj, const ANIM_FRAME *frame, uint32_t mesh_bits);
