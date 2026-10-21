#pragma once

#include <trx/game/lara/enum.h>
#include <trx/game/types.h>

// Points the laser sight of a weapon along a line, from the eye of the
// laser sight view to where it looks, and places the red dot where the line
// first meets something. A shot hits what the dot is on: a creature takes
// the weapon's damage, a smashable item breaks, and a wall shows a
// ricochet.
void Gun_LaserSight_Aim(
    LARA_GUN_TYPE gun_type, GAME_VECTOR start, GAME_VECTOR target, bool fire);
