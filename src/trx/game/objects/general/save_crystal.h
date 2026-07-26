#pragma once

#include <trx/game/objects/ids.h>

#include <stdint.h>

// Mesh bits the crystal is drawn with, for either the level item or its
// inventory counterpart. A negative mesh_index lets the save crystal mode pick
// the tint.
uint32_t SaveCrystal_GetMeshBits(OBJECT_ID object_id, int32_t mesh_index);
