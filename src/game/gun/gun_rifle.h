#pragma once

// Private gun routines related to rifles.

#include "global/types.h"

void Gun_Rifle_Draw(void);
void Gun_Rifle_Undraw(void);
void Gun_Rifle_DrawMeshes(void);
void Gun_Rifle_UndrawMeshes(void);
void Gun_Rifle_Ready(void);
void Gun_Rifle_Control(LARA_GUN_TYPE weapon_type);
void Gun_Rifle_Animate(void);
void Gun_Rifle_Fire(void);
