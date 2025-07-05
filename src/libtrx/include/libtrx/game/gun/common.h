#pragma once

#include "../lara/enum.h"

extern void Gun_InitialiseNewWeapon(void);
extern void Gun_Control(void);

extern void Gun_SetLaraBackMesh(LARA_GUN_TYPE weapon_type);
extern void Gun_SetLaraHolsterLMesh(LARA_GUN_TYPE weapon_type);
extern void Gun_SetLaraHolsterRMesh(LARA_GUN_TYPE weapon_type);
