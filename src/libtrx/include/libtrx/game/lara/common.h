#pragma once

#include "../collision.h"
#include "../game_flow.h"
#include "../items.h"
#include "types.h"

LARA_INFO *Lara_GetLaraInfo(void);
ITEM *Lara_GetItem(void);
void Lara_Initialise(const GF_LEVEL *level);
extern void Lara_InitialiseLoad(int16_t item_num);
void Lara_InitialiseInventory(const GF_LEVEL *level);
void Lara_RevertToPistolsIfNeeded(void);
void Lara_UseItem(GAME_OBJECT_ID obj_id);
void Lara_SetStartAnimState(LARA_EXTRA_STATE state);
bool Lara_IsControllable(void);
void Lara_SetControllable(bool controllable);
ITEM *Lara_GetDeathCameraTarget(void);
void Lara_SetDeathCameraTarget(int16_t item_num);
GAME_OBJECT_ID Lara_GetAnimationObject(void);
void Lara_Animate(ITEM *item);
void Lara_AnimateUntil(ITEM *lara_item, int32_t goal);
const ANIM_FRAME *Lara_GetHitFrame(const ITEM *item);
void Lara_TakeDamage(int16_t damage, bool hit_status);

bool Lara_TestBoundsCollide(const ITEM *item, int32_t radius);
void Lara_Push(const ITEM *item, COLL_INFO *coll, bool hit_on, bool big_push);
bool Lara_TestPosition(const ITEM *item, const OBJECT_BOUNDS *bounds);
void Lara_AlignPosition(const ITEM *item, const XYZ_32 *vec);
bool Lara_MovePosition(const ITEM *item, const XYZ_32 *vec);
bool Lara_IsNearItem(const XYZ_32 *pos, int32_t distance);
