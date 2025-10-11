#pragma once

#include "global/types.h"

#include <libtrx/game/objects/common.h>

BOUNDS_16 Object_GetBoundingBox(
    const OBJECT *obj, const ANIM_FRAME *frame, uint32_t mesh_bits);
