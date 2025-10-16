#pragma once

#include "../collision.h"
#include "../game_flow.h"
#include "../items.h"
#include "enum.h"
#include "types.h"

#define LA(anim) Lara_AnimToGameID(anim)
#define LA_U(anim) Lara_AnimFromGameID(anim)
#define LS(state) Lara_StateToGameID(state)
#define LS_U(state) Lara_StateFromGameID(state)

LARA_INFO *Lara_GetLaraInfo(void);
ITEM *Lara_GetItem(void);
void Lara_Initialise(const GF_LEVEL *level);
extern void Lara_InitialiseLoad(int16_t item_num);
void Lara_InitialiseInventory(const GF_LEVEL *level);
void Lara_RevertToPistolsIfNeeded(void);
void Lara_UseItem(OBJECT_ID obj_id);
void Lara_SetStartAnimState(LARA_EXTRA_STATE state);
bool Lara_IsControllable(void);
void Lara_SetControllable(bool controllable);
bool Lara_CanInterpolate(const ITEM *item, int32_t frame_a, int32_t frame_b);
ITEM *Lara_GetDeathCameraTarget(void);
void Lara_SetDeathCameraTarget(int16_t item_num);
OBJECT_ID Lara_GetAnimationObject(void);
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

LARA_ANIMATION Lara_AnimToGameID(LARA_TRX_ANIMATION anim);
LARA_STATE Lara_StateToGameID(LARA_TRX_STATE state);
LARA_TRX_ANIMATION Lara_AnimFromGameID(LARA_ANIMATION anim);
LARA_TRX_STATE Lara_StateFromGameID(LARA_STATE state);
