#pragma once

#include "../lara/types.h"

extern void Gun_InitialiseNewWeapon(void);
extern void Gun_Control(void);

void Gun_SetLaraBackMesh(LARA_GUN_TYPE weapon_type);
void Gun_SetLaraHandLMesh(LARA_GUN_TYPE weapon_type);
void Gun_SetLaraHandRMesh(LARA_GUN_TYPE weapon_type);
void Gun_SetLaraHolsterLMesh(LARA_GUN_TYPE weapon_type);
void Gun_SetLaraHolsterRMesh(LARA_GUN_TYPE weapon_type);

// TODO: make this a struct
GAME_OBJECT_ID Gun_GetWeaponAnim(LARA_GUN_TYPE gun_type);
LARA_GUN_TYPE Gun_GetType(GAME_OBJECT_ID obj_id);
GAME_OBJECT_ID Gun_GetGunObject(LARA_GUN_TYPE gun_type);
GAME_OBJECT_ID Gun_GetAmmoObject(LARA_GUN_TYPE gun_type);
int32_t Gun_GetAmmoQuantity(LARA_GUN_TYPE gun_type);
AMMO_INFO *Gun_GetAmmoInfo(LARA_GUN_TYPE gun_type);
