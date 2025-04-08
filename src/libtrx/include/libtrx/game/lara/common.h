#pragma once

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
