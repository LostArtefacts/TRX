#pragma once

// Public gun routines.

#include "global/types.h"

#include <stdint.h>

void Gun_Control(void);
void Gun_InitialiseNewWeapon(void);
void Gun_AimWeapon(WEAPON_INFO *winfo, LARA_ARM *arm);
int32_t Gun_FireWeapon(
    int32_t weapon_type, ITEM_INFO *target, ITEM_INFO *src, PHD_ANGLE *angles);
void Gun_HitTarget(ITEM_INFO *item, GAME_VECTOR *hitpos, int16_t damage);
void Gun_DrawFlash(LARA_GUN_TYPE weapon_type, int32_t clip);
GAME_OBJECT_ID Gun_GetLaraAnimation(LARA_GUN_TYPE gun_type);
