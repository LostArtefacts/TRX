#pragma once

// Private gun routines.

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

extern WEAPON_INFO g_Weapons[NUM_WEAPONS];

void Gun_TargetInfo(WEAPON_INFO *winfo);
void Gun_GetNewTarget(WEAPON_INFO *winfo);
void Gun_FindTargetPoint(ITEM_INFO *item, GAME_VECTOR *target);
void Gun_AimWeapon(WEAPON_INFO *winfo, LARA_ARM *arm);
int32_t Gun_FireWeapon(
    int32_t weapon_type, ITEM_INFO *target, ITEM_INFO *src, PHD_ANGLE *angles);
void Gun_HitTarget(ITEM_INFO *item, GAME_VECTOR *hitpos, int16_t damage);

void Gun_DrawFlash(LARA_GUN_TYPE weapon_type, int32_t clip);
