#pragma once

// Private gun routines.

#include "global/types.h"

#include <libtrx/game/gun/misc.h>
#include <libtrx/game/gun/types.h>
#include <libtrx/game/items/types.h>

#include <stdint.h>

void Gun_TargetInfo(WEAPON_INFO *winfo);
void Gun_GetNewTarget(WEAPON_INFO *winfo);
void Gun_ChangeTarget(WEAPON_INFO *winfo);
void Gun_AimWeapon(WEAPON_INFO *winfo, LARA_ARM *arm);
void Gun_HitTarget(ITEM *item, GAME_VECTOR *hitpos, int16_t damage);

void Gun_DrawFlash(LARA_GUN_TYPE weapon_type, int32_t clip);
