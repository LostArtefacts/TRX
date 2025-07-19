#pragma once

#include "../items/types.h"
#include "../lara/types.h"
#include "../types.h"
#include "./types.h"

typedef enum {
    PROJECTILE_HIT_NONE,
    PROJECTILE_HIT_SHATTER, // some objects were shattered
    PROJECTILE_HIT_STOP, // a hard object (eg a bell) has been hit
} PROJECTILE_HIT;

void Gun_FindTargetPoint(const ITEM *item, GAME_VECTOR *target);
void Gun_AimWeapon(const WEAPON_INFO *weapon, LARA_ARM *arm);
void Gun_TargetInfo(const WEAPON_INFO *weapon);

#if TR_VERSION >= 2
extern PROJECTILE_HIT Gun_SmashItems(GAME_VECTOR start, GAME_VECTOR target);
#endif
