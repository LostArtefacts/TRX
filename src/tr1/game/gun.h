#pragma once

// Public gun routines.

#include "global/types.h"

#include <libtrx/game/gun.h>

void Gun_AimWeapon(WEAPON_INFO *winfo, LARA_ARM *arm);
int32_t Gun_FireWeapon(
    int32_t weapon_type, ITEM *target, ITEM *src, PHD_ANGLE *angles);
void Gun_HitTarget(ITEM *item, GAME_VECTOR *hitpos, int16_t damage);
void Gun_DrawFlash(LARA_GUN_TYPE weapon_type, int32_t clip);
GAME_OBJECT_ID Gun_GetLaraAnim(LARA_GUN_TYPE gun_type);
void Gun_UpdateLaraMeshes(GAME_OBJECT_ID obj_id);
void Gun_SetLaraHandLMesh(LARA_GUN_TYPE weapon_type);
void Gun_SetLaraHandRMesh(LARA_GUN_TYPE weapon_type);
