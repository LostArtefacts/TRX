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
} SHATTER_ITEM;

void FX_Debris_ShatterItem(
    const SHATTER_ITEM *shatter_item, int16_t face_count, int16_t room_num,
    int32_t xz_vel);
void FX_Debris_ShatterStatic(
    const STATIC_MESH *static_mesh, int16_t face_count, int16_t room_num,
    int32_t xz_vel);
