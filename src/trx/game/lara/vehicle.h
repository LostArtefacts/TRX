#pragma once

#include <trx/game/items/types.h>
#include <trx/game/lara/enum.h>

bool Lara_Vehicle_IsMounted(void);
bool Lara_Vehicle_IsOnType(OBJECT_ID obj_id);
void Lara_Vehicle_SetIndex(int16_t item_num);
int16_t Lara_Vehicle_GetIndex(void);
ITEM *Lara_Vehicle_GetItem(void);

// Returns the weapon the vehicle Lara rides shoots with, and LGT_UNARMED
// where she rides none or it carries no weapon.
LARA_GUN_TYPE Lara_Vehicle_GetGunType(void);

OBJECT_ID Lara_Vehicle_GetAnimationObject(void);
void Lara_Vehicle_SwitchToAnim(int16_t anim_idx, int16_t frame_idx);
int16_t Lara_Vehicle_GetRelativeAnim(void);
bool Lara_Vehicle_TestAnimEqual(int16_t anim_idx);
void Lara_Vehicle_SyncItemAnim(void);

void Lara_Vehicle_Dismount(void);
