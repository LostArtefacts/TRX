#pragma once

// Private gun routines.

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
void Gun_UpdateLaraMeshes(OBJECT_ID obj_id);

void Gun_GetNewTarget(const WEAPON_INFO *weapon);
void Gun_ChangeTarget(const WEAPON_INFO *weapon);
void Gun_HitTarget(ITEM *item, const GAME_VECTOR *hit_pos, int32_t damage);

void Gun_DrawFlash(LARA_GUN_TYPE weapon_type, CLIP clip);

PROJECTILE_HIT Gun_SmashItems(XYZ_32 start, XYZ_32 target, XYZ_32 *out_hit_pos);
