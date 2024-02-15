#pragma once

// Public Lara routines.

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

void Lara_Control(void);

void Lara_ControlExtra(int16_t item_num);
void Lara_Animate(ITEM_INFO *item);
void Lara_AnimateUntil(ITEM_INFO *lara_item, int32_t goal);

void Lara_Initialise(int32_t level_num);
void Lara_InitialiseLoad(int16_t item_num);
void Lara_InitialiseInventory(int32_t level_num);
void Lara_InitialiseMeshes(int32_t level_num);

void Lara_SwapMeshExtra(void);
bool Lara_IsNearItem(const VECTOR_3D *pos, int32_t distance);
void Lara_UseItem(int16_t object_num);

bool Lara_TestBoundsCollide(ITEM_INFO *item, int32_t radius);
bool Lara_TestPosition(ITEM_INFO *item, int16_t *bounds);
void Lara_AlignPosition(ITEM_INFO *item, VECTOR_3D *vec);
bool Lara_MovePosition(ITEM_INFO *item, VECTOR_3D *vec);
void Lara_Push(ITEM_INFO *item, COLL_INFO *coll, bool spaz_on, bool big_push);

void Lara_TakeDamage(int16_t damage, bool hit_status);
void Lara_EnterFlyMode(void);
