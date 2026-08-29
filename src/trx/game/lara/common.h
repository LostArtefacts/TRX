#pragma once

#include <trx/game/collision.h>
#include <trx/game/game_flow.h>
#include <trx/game/items.h>
#include <trx/game/lara/enum.h>
#include <trx/game/lara/types.h>

#define LA(anim) Lara_AnimIDToSlot(anim)
#define LA_U(anim) Lara_AnimSlotToID(anim)
#define LS(state) Lara_StateIDToSlot(state)
#define LS_U(state) Lara_StateSlotToID(state)

LARA_INFO *Lara_GetLaraInfo(void);
ITEM *Lara_GetItem(void);
void Lara_Initialise(const GF_LEVEL *level);
void Lara_InitialiseLoad(int16_t item_num);
void Lara_InitialiseInventory(const GF_LEVEL *level);
void Lara_RevertToDefaultGunIfNeeded(void);
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
void Lara_Kill(void);

bool Lara_GetMeshPos(LARA_MESH mesh, XYZ_32 *out_pos);
bool Lara_TestBoundsCollide(const ITEM *item, int32_t radius);
bool Lara_TestPosition(const ITEM *item, const OBJECT_BOUNDS *bounds);
void Lara_AlignPosition(const ITEM *item, const XYZ_32 *vec);
bool Lara_MovePosition(const ITEM *item, const XYZ_32 *vec);
bool Lara_MovePositionEx(
    const ITEM *item, const XYZ_32 *vec, int16_t extra_y_rot);
bool Lara_IsNearItem(const XYZ_32 *pos, int32_t distance);

LARA_ANIMATION_SLOT Lara_AnimIDToSlot(LARA_ANIMATION_ID anim);
LARA_STATE_SLOT Lara_StateIDToSlot(LARA_STATE_ID state);
LARA_ANIMATION_ID Lara_AnimSlotToID(LARA_ANIMATION_SLOT anim);
LARA_STATE_ID Lara_StateSlotToID(LARA_STATE_SLOT state);
