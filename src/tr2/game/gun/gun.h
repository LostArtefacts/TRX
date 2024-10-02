#pragma once

#include "global/types.h"

void __cdecl Gun_Control(void);
void __cdecl Gun_InitialiseNewWeapon(void);
int32_t __cdecl Gun_GetWeaponAnim(LARA_GUN_TYPE gun_type);

// TODO: make this a struct
GAME_OBJECT_ID Gun_GetGunObject(LARA_GUN_TYPE gun_type);
GAME_OBJECT_ID Gun_GetAmmoObject(LARA_GUN_TYPE gun_type);
int32_t Gun_GetAmmoQuantity(LARA_GUN_TYPE gun_type);
AMMO_INFO *Gun_GetAmmoInfo(LARA_GUN_TYPE gun_type);

void Gun_SetLaraBackMesh(LARA_GUN_TYPE weapon_type);
void Gun_SetLaraHandLMesh(LARA_GUN_TYPE weapon_type);
void Gun_SetLaraHandRMesh(LARA_GUN_TYPE weapon_type);
void Gun_SetLaraHolsterLMesh(LARA_GUN_TYPE weapon_type);
void Gun_SetLaraHolsterRMesh(LARA_GUN_TYPE weapon_type);
