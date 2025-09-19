#pragma once

// Private gun routines.

#include "global/types.h"

#include <libtrx/game/gun/misc.h>
#include <libtrx/game/gun/types.h>
#include <libtrx/game/items/types.h>

#include <stdint.h>

void Gun_GetNewTarget(WEAPON_INFO *weapon);
void Gun_ChangeTarget(WEAPON_INFO *weapon);
void Gun_HitTarget(ITEM *item, GAME_VECTOR *hitpos, int16_t damage);

void Gun_DrawFlash(LARA_GUN_TYPE weapon_type, CLIP clip);
