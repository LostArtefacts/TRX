#pragma once

// Public gun routines.

#include "global/types.h"

#include <stdint.h>

void Gun_Control(void);
void Gun_InitialiseNewWeapon(void);
void Gun_AimWeapon(WEAPON_INFO *winfo, LARA_ARM *arm);
int32_t Gun_FireWeapon(
    int32_t weapon_type, ITEM *target, ITEM *src, PHD_ANGLE *angles);
void Gun_HitTarget(ITEM *item, GAME_VECTOR *hitpos, int16_t damage);
void Gun_DrawFlash(LARA_GUN_TYPE weapon_type, int32_t clip);
GAME_OBJECT_ID Gun_GetLaraAnim(LARA_GUN_TYPE gun_type);
GAME_OBJECT_ID Gun_GetWeaponAnim(LARA_GUN_TYPE gun_type);
LARA_GUN_TYPE Gun_GetType(GAME_OBJECT_ID object_id);
void Gun_UpdateLaraMeshes(GAME_OBJECT_ID object_id);
void Gun_SetLaraBackMesh(LARA_GUN_TYPE weapon_type);
void Gun_SetLaraHandLMesh(LARA_GUN_TYPE weapon_type);
void Gun_SetLaraHandRMesh(LARA_GUN_TYPE weapon_type);
void Gun_SetLaraHolsterLMesh(LARA_GUN_TYPE weapon_type);
void Gun_SetLaraHolsterRMesh(LARA_GUN_TYPE weapon_type);
