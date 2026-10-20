#pragma once

#include <trx/game/items/types.h>
#include <trx/game/objects/types.h>

// Chooses the mesh drawn at each joint of an item's object.
typedef struct {
    // Joints drawn with the object's own mesh.
    uint32_t mesh_mask;
    // Draws the joints outside mesh_mask with this object's meshes. Null
    // leaves those joints out.
    const OBJECT *mesh_swap;
} ITEM_DRAW_DESC;

void Item_ControlDraw(ITEM *item);

bool Item_DrawAnimating(const ITEM *item);
// Draws the item's object animated with the given choice of meshes. A swap
// object that the level does not have leaves those joints out.
bool Item_DrawAnimatingEx(const ITEM *item, const ITEM_DRAW_DESC *desc);
bool Item_DrawUnclipped(const ITEM *item);
bool Item_DrawScaled(const ITEM *item, XYZ_32 scale, XYZ_32 prev_scale);
bool Item_DrawSprite(const ITEM *item);
bool Item_DrawPickup(const ITEM *item);
