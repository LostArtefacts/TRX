#pragma once

#include <trx/game/items/types.h>
#include <trx/game/objects/types.h>

void Item_ControlDraw(ITEM *item);

bool Item_DrawAnimating(const ITEM *item);
bool Item_DrawAnimatingWithSwap(
    const ITEM *item, const OBJECT *mesh_swap, uint32_t mesh_mask);
bool Item_DrawUnclipped(const ITEM *item);
bool Item_DrawScaled(const ITEM *item, XYZ_32 scale, XYZ_32 prev_scale);
bool Item_DrawSprite(const ITEM *item);
bool Item_DrawPickup(const ITEM *item);
