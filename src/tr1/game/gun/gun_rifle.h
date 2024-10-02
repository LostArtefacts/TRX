#pragma once

// Private gun routines related to two-handed weapons.
//
// In Tomb Raider 1 this means only shotgun. Future games add more weapons such
// as M-16, Harpoon, Grenade Launcher etc.

#include "global/types.h"

void Gun_Rifle_Draw(LARA_GUN_TYPE weapon_type);
void Gun_Rifle_Undraw(LARA_GUN_TYPE weapon_type);
void Gun_Rifle_DrawMeshes(LARA_GUN_TYPE weapon_type);
void Gun_Rifle_UndrawMeshes(LARA_GUN_TYPE weapon_type);
void Gun_Rifle_Ready(LARA_GUN_TYPE weapon_type);
void Gun_Rifle_Control(LARA_GUN_TYPE weapon_type);
void Gun_Rifle_Animate(LARA_GUN_TYPE weapon_type);
void Gun_Rifle_Fire(LARA_GUN_TYPE weapon_type);
