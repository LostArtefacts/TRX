// An object is the definition every item of that type is cut from - a wolf's
// radius, not this wolf's. Per-item state lives on the item; see trx.items.

#include <trx/core/field.h>
#include <trx/game/objects/types.h>

// clang-format off
static const FIELD_DESC M_OBJECT_FIELDS[] = {
    // what the level actually has
    FIELD_RO(OBJECT, loaded),
    FIELD_RO(OBJECT, intelligent),
    FIELD_RO(OBJECT, mesh_count),
    FIELD_RO(OBJECT, anim_count),

    // the numbers that decide how it behaves
    FIELD(OBJECT, radius),
    FIELD(OBJECT, shadow_size),
    FIELD(OBJECT, smartness),
    FIELD(OBJECT, pivot_length),
    FIELD(OBJECT, semi_transparent),

    // Deliberately absent: every _func pointer, the mesh and animation indices,
    // and the frame data. They are how the engine drives an object, and a script
    // moving them would point it at the wrong meshes.
};
// clang-format on

TYPE_DEFINE(OBJECT, M_OBJECT_FIELDS)
