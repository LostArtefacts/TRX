#pragma once

// Public gun routines.

#include "global/types.h"

#include <libtrx/game/gun.h>

void Gun_HitTarget(ITEM *item, GAME_VECTOR *hitpos, int16_t damage);
void Gun_DrawFlash(LARA_GUN_TYPE weapon_type, CLIP clip);
void Gun_UpdateLaraMeshes(GAME_OBJECT_ID obj_id);
