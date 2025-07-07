#pragma once

#include "global/types.h"

void Gun_Pistols_SetArmInfo(LARA_ARM *arm, int32_t frame);
void Gun_Pistols_Draw(LARA_GUN_TYPE weapon_type);
void Gun_Pistols_Undraw(LARA_GUN_TYPE weapon_type);
void Gun_Pistols_Ready(LARA_GUN_TYPE weapon_type);
void Gun_Pistols_Control(LARA_GUN_TYPE weapon_type);
void Gun_Pistols_Animate(LARA_GUN_TYPE weapon_type);
