#pragma once

#include <trx/core/math.h>
#include <trx/game/anims.h>
#include <trx/game/collision.h>
#include <trx/game/items.h>
#include <trx/game/objects/ids.h>
#include <trx/game/objects/types.h>

// Returns how the bounds clip against the view, or fully visible for an
// object that opts out of clipping.
CLIP Object_GetClip(const OBJECT *obj, const BOUNDS_16 *bounds);
void Object_DrawMesh(int32_t mesh_idx, CLIP clip, bool interpolated);
void Object_DrawStaticObject(
    const OBJECT *obj, const ANIM_FRAME *frame, uint32_t mesh_mask);

bool Object_DrawInterpolatedObject(
    const OBJECT *obj, uint32_t mesh_mask, const int16_t *extra_rotation,
    const ANIM_FRAME *frame1, const ANIM_FRAME *frame2, int32_t frac,
    int32_t rate);

bool Object_DrawInterpolatedObjectWithSwap(
    const OBJECT *obj, uint32_t mesh_mask, const int16_t *extra_rotation,
    const ANIM_FRAME *frame1, const ANIM_FRAME *frame2, int32_t frac,
    int32_t rate, const OBJECT *mesh_swap);

// Draws the object about its pivot without the frame offset. Some TR4 inventory
// models have an offset that places them far from the pivot.
bool Object_DrawInterpolatedObjectAtPivot(
    const OBJECT *obj, uint32_t mesh_mask, const int16_t *extra_rotation,
    const ANIM_FRAME *frame1, const ANIM_FRAME *frame2, int32_t frac,
    int32_t rate);

void Object_ApplyExtraRotation(
    const int16_t **extra_rotation, const XYZ_BOOL rot_flags,
    bool interpolated);
