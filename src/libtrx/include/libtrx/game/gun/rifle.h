#pragma once

#include "../lara/types.h"

extern void Gun_Rifle_Control(LARA_GUN_TYPE weapon_type);
extern void Gun_Rifle_Draw(LARA_GUN_TYPE weapon_type);
extern void Gun_Rifle_Undraw(LARA_GUN_TYPE weapon_type);

void Gun_Rifle_Fire(LARA_GUN_TYPE weapon_type, bool running);

void Gun_Rifle_DrawMeshes(LARA_GUN_TYPE weapon_type);
void Gun_Rifle_UndrawMeshes(LARA_GUN_TYPE weapon_type);
