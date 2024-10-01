#pragma once

#include "global/types.h"

void __cdecl Gun_Rifle_DrawMeshes(LARA_GUN_TYPE weapon_type);
void __cdecl Gun_Rifle_UndrawMeshes(LARA_GUN_TYPE weapon_type);
void __cdecl Gun_Rifle_Ready(LARA_GUN_TYPE weapon_type);
void __cdecl Gun_Rifle_Control(LARA_GUN_TYPE weapon_type);
void __cdecl Gun_Rifle_FireShotgun(void);
void __cdecl Gun_Rifle_FireM16(bool running);
void __cdecl Gun_Rifle_FireHarpoon(void);
void __cdecl Gun_Rifle_FireGrenade(void);
void __cdecl Gun_Rifle_Draw(LARA_GUN_TYPE weapon_type);
void __cdecl Gun_Rifle_Undraw(LARA_GUN_TYPE weapon_type);
void __cdecl Gun_Rifle_Animate(LARA_GUN_TYPE weapon_type);
