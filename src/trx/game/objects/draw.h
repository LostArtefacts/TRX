#pragma once

#include <trx/core/math.h>
#include <trx/game/anims.h>
#include <trx/game/collision.h>
#include <trx/game/objects/ids.h>
#include <trx/game/objects/types.h>

// Returns how the bounds clip against the view, or fully visible for an
// object that opts out of clipping.
CLIP Object_GetClip(const OBJECT *obj, const BOUNDS_16 *bounds);

void Object_DrawMesh(int32_t mesh_idx, CLIP clip, bool interpolated);

void Object_DrawStaticObject(
    const OBJECT *obj, const ANIM_FRAME *frame, uint32_t mesh_mask);

void Object_ApplyExtraRotation(
    const int16_t **extra_rotation, const XYZ_BOOL rot_flags,
    bool interpolated);
