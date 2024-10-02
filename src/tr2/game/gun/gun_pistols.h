#pragma once

#include "global/types.h"

void __cdecl Gun_Pistols_SetArmInfo(LARA_ARM *arm, int32_t frame);
void __cdecl Gun_Pistols_Draw(LARA_GUN_TYPE weapon_type);
void __cdecl Gun_Pistols_Undraw(LARA_GUN_TYPE weapon_type);
void __cdecl Gun_Pistols_Ready(LARA_GUN_TYPE weapon_type);
void __cdecl Gun_Pistols_DrawMeshes(LARA_GUN_TYPE weapon_type);
void __cdecl Gun_Pistols_UndrawMeshLeft(LARA_GUN_TYPE weapon_type);
void __cdecl Gun_Pistols_UndrawMeshRight(LARA_GUN_TYPE weapon_type);
void __cdecl Gun_Pistols_Control(LARA_GUN_TYPE weapon_type);
void __cdecl Gun_Pistols_Animate(LARA_GUN_TYPE weapon_type);
