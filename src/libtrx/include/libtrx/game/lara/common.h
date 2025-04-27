#pragma once

#include "../collision.h"
#include "../items.h"
#include "types.h"

LARA_INFO *Lara_GetLaraInfo(void);
ITEM *Lara_GetItem(void);
void Lara_Animate(ITEM *item);
void Lara_SwapSingleMesh(LARA_MESH mesh, GAME_OBJECT_ID obj_id);
OBJECT_MESH *Lara_GetMesh(LARA_MESH mesh);
void Lara_SetMesh(LARA_MESH mesh, OBJECT_MESH *mesh_ptr);
const ANIM_FRAME *Lara_GetHitFrame(const ITEM *item);
void Lara_TakeDamage(int16_t damage, bool hit_status);

bool Lara_TestBoundsCollide(const ITEM *item, int32_t radius);
void Lara_Push(const ITEM *item, COLL_INFO *coll, bool hit_on, bool big_push);
bool Lara_TestPosition(const ITEM *item, const OBJECT_BOUNDS *bounds);
void Lara_AlignPosition(const ITEM *item, const XYZ_32 *vec);
bool Lara_IsNearItem(const XYZ_32 *pos, int32_t distance);
