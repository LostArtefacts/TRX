#pragma once

// Private gun routines related to two-handed weapons.
//
// In Tomb Raider 1 this means only shotgun. Future games add more weapons such
// as M-16, Harpoon, Grenade Launcher etc.

#include <libtrx/game/gun/rifle.h>

void Gun_Rifle_Animate(LARA_GUN_TYPE weapon_type);
void Gun_Rifle_Ready(LARA_GUN_TYPE weapon_type);
