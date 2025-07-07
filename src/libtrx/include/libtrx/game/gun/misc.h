#pragma once

#include "../items/types.h"
#include "../lara/types.h"
#include "../types.h"
#include "./types.h"

void Gun_FindTargetPoint(const ITEM *item, GAME_VECTOR *target);
void Gun_AimWeapon(const WEAPON_INFO *winfo, LARA_ARM *arm);
