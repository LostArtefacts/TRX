#pragma once

#include "../anims.h"
#include "../collision.h"
#include "../items.h"
#include "../math.h"
#include "ids.h"
#include "types.h"

void Object_DrawUnclippedItem(const ITEM *item);
void Object_DrawMesh(int32_t mesh_idx, CLIP clip, bool interpolated);
extern void Object_DrawSpriteItem(const ITEM *item);
extern void Object_DrawPickupItem(const ITEM *item);
void Object_DrawStaticObject(const OBJECT *obj, const ANIM_FRAME *frame);

void Object_DrawAnimatingItem(const ITEM *item);
void Object_DrawInterpolatedObject(
    const OBJECT *obj, uint32_t meshes, const int16_t *extra_rotation,
    const ANIM_FRAME *frame1, const ANIM_FRAME *frame2, int32_t frac,
    int32_t rate);

void Object_ApplyExtraRotation(
    const int16_t **extra_rotation, const XYZ_BOOL rot_flags,
    bool interpolated);
