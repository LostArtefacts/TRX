#pragma once

#include <trx/game/collision/types.h>
#include <trx/game/objects/types.h>
#include <trx/game/rooms/types.h>

typedef struct {
    XYZ_32 pos;
    OBJECT_MESH *mesh;
    int32_t bit;
    int16_t yaw;
    int16_t flags;
    // Throws the pieces in random directions and spins them from the start,
    // rather than out from the centre of the mesh.
    bool scatter;
} SHATTER_ITEM;

void FX_Debris_ShatterItem(
    const SHATTER_ITEM *shatter_item, int16_t face_count, int16_t room_num,
    int32_t xz_vel);
// Breaks one mesh of the item into debris at its joint and hides the mesh.
// A negative face count skips faces at random, up to the same count. Returns
// whether the mesh was showing.
bool FX_Debris_ShatterItemMesh(
    ITEM *item, int32_t mesh_idx, int16_t face_count, int32_t xz_vel,
    bool scatter);
void FX_Debris_ShatterStatic(
    const STATIC_MESH *static_mesh, int16_t face_count, int16_t room_num,
    int32_t xz_vel);
