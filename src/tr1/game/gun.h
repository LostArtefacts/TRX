#pragma once

// Public gun routines.

#include "global/types.h"

#include <libtrx/game/gun.h>

void Gun_AimWeapon(WEAPON_INFO *winfo, LARA_ARM *arm);
void Gun_HitTarget(ITEM *item, GAME_VECTOR *hitpos, int16_t damage);
void Gun_DrawFlash(LARA_GUN_TYPE weapon_type, int32_t clip);
GAME_OBJECT_ID Gun_GetLaraAnim(LARA_GUN_TYPE gun_type);
void Gun_UpdateLaraMeshes(GAME_OBJECT_ID obj_id);
