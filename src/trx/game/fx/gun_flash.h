#pragma once

#include <trx/game/creature/types.h>
#include <trx/game/items/types.h>

// Draws a muzzle flash at one of an item's joints for a few frames, and
// lights the area around it where gun lighting is on. The bite names the
// joint and the offset from it that the flash sits at, and rot_x tilts the
// flash mesh around that point.
bool FX_GunFlash_SpawnAt(
    const ITEM *owner_item, BITE bite, OBJECT_ID flash_obj_id, int16_t rot_x);

// Draws a muzzle flash for a firing creature, taking the joint, the tilt and
// the flash mesh from the weapon it holds. Reports failure where the weapon
// has no flash to draw.
bool FX_GunFlash_Spawn(const ITEM *owner_item, const CREATURE_GUN *gun);
