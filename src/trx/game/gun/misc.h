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

// Hurts an item caught in a TR4 explosion, such as a grenade's or an
// explosive bolt's, set off by source. Lara takes a fixed blow and catches
// fire where she is low on health and out of water, and a creature that dies
// is blown apart.
void Gun_HitTargetWithBlast(ITEM *item, const ITEM *source);

void Gun_DrawFlash(LARA_GUN_TYPE weapon_type, CLIP clip, bool interpolated);

// Draws the flash with the two hand positions exchanged, as a weapon held in
// the other hand shows it.
void Gun_DrawFlashMirrored(LARA_GUN_TYPE weapon_type, CLIP clip);
