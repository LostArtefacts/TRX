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
bool Lara_IsNearItem(const XYZ_32 *pos, int32_t distance);
void Lara_UseItem(int16_t object_num);
int16_t Lara_GetNearestEnemy(void);

bool Lara_TestBoundsCollide(ITEM_INFO *item, int32_t radius);
bool Lara_TestPosition(const ITEM_INFO *item, const OBJECT_BOUNDS *bounds);
void Lara_AlignPosition(ITEM_INFO *item, XYZ_32 *vec);
bool Lara_MovePosition(ITEM_INFO *item, XYZ_32 *vec);
void Lara_Push(ITEM_INFO *item, COLL_INFO *coll, bool spaz_on, bool big_push);

void Lara_TakeDamage(int16_t damage, bool hit_status);
void Lara_EnterFlyMode(void);
