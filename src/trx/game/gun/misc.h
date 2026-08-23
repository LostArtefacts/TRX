#pragma once

// Private gun routines.

#include <trx/game/gun/types.h>
#include <trx/game/items/types.h>
#include <trx/game/lara/types.h>
#include <trx/game/types.h>

void Gun_FindTargetPoint(const ITEM *item, GAME_VECTOR *target);
void Gun_AimWeapon(const WEAPON_INFO *weapon, LARA_ARM *arm);
void Gun_TargetInfo(const WEAPON_INFO *weapon);
void Gun_UpdateLaraMeshes(OBJECT_ID obj_id);

void Gun_GetNewTarget(const WEAPON_INFO *weapon);
void Gun_ChangeTarget(const WEAPON_INFO *weapon);
void Gun_HitTarget(
    ITEM *item, const GAME_VECTOR *start, const GAME_VECTOR *hit_pos,
    int32_t damage);

void Gun_DrawFlash(LARA_GUN_TYPE weapon_type, CLIP clip, bool interpolated);

// Draws the flash with the two hand positions exchanged, as a weapon held in
// the other hand shows it.
void Gun_DrawFlashMirrored(LARA_GUN_TYPE weapon_type, CLIP clip);

// Marks the TR1/TR2 muzzle flash and flare fire meshes semi-transparent per
// the gun glow setting, matching the PS1 versions.
void Gun_ApplyFlashSemiTransparency(void);
