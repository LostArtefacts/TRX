#pragma once

#include <trx/game/gun/types.h>
#include <trx/game/input/enum.h>
#include <trx/game/inventory.h>
#include <trx/game/lara/types.h>

void Gun_InitialiseNewWeapon(void);

void Gun_SetLaraBackMesh(LARA_GUN_TYPE weapon_type);
void Gun_SetLaraHandLMesh(LARA_GUN_TYPE weapon_type);
void Gun_SetLaraHandRMesh(LARA_GUN_TYPE weapon_type);
void Gun_SetLaraHolsterLMesh(LARA_GUN_TYPE weapon_type);
void Gun_SetLaraHolsterRMesh(LARA_GUN_TYPE weapon_type);

// TODO: make this a struct
OBJECT_ID Gun_GetLaraAnim(LARA_GUN_TYPE gun_type);
OBJECT_ID Gun_GetWeaponAnim(LARA_GUN_TYPE gun_type);
LARA_GUN_TYPE Gun_GetType(OBJECT_ID obj_id);
OBJECT_ID Gun_GetGunObject(LARA_GUN_TYPE gun_type);
// Returns the weapon an object is the pickup for, and LGT_UNARMED where the
// object is the pickup for none.
LARA_GUN_TYPE Gun_GetTypeForObject(OBJECT_ID obj_id);
// Returns the weapon a key draws straight away, and LGT_UNARMED where the key
// draws none.
LARA_GUN_TYPE Gun_GetTypeForInputRole(INPUT_ROLE role);
OBJECT_ID Gun_GetAmmoObject(LARA_GUN_TYPE gun_type);
// Ammunition is counted in rounds, one of which is what a single shot at a
// target spends. The shotgun fires six of them at once.
int32_t Gun_GetInitialRounds(LARA_GUN_TYPE gun_type);
int32_t Gun_GetRoundsPerBox(LARA_GUN_TYPE gun_type);
int32_t Gun_GetRoundsPerShot(LARA_GUN_TYPE gun_type);
int32_t Gun_GetAmmoInventoryQuantity(LARA_GUN_TYPE gun_type);
// Which weapon hangs in Lara's holsters and which on her back, out of what an
// inventory holds. More than one can qualify, and these say which wins.
LARA_GUN_TYPE Gun_GetHolsterChoice(const INVENTORY_STATE *inv);
LARA_GUN_TYPE Gun_GetBackChoice(const INVENTORY_STATE *inv);

// Whether the weapon spends nothing when it fires, so that its rounds are
// neither counted down nor shown to the player.
bool Gun_HasInfiniteAmmo(LARA_GUN_TYPE gun_type);
// Whether the weapon has anything left to fire. One that never runs out
// always has.
bool Gun_HasRoundsLeft(LARA_GUN_TYPE gun_type);
// Takes the round a shot costs, from a weapon that spends any.
void Gun_SpendRound(LARA_GUN_TYPE gun_type);

bool Gun_IsRifleType(LARA_GUN_TYPE gun_type);
// Whether the weapon keeps firing while the trigger is held, which also lets
// Lara fire it on the move.
bool Gun_IsMachineGunType(LARA_GUN_TYPE gun_type);
// Whether the game holds a machine gun the player can find.
bool Gun_HasAvailableMachineGun(void);
// Whether the game holds a launcher the player can find.
bool Gun_HasAvailableLauncher(void);
// Returns the weapon Lara starts the game with and reaches for when what she
// holds runs dry, and LGT_UNARMED where no weapon claims the place.
LARA_GUN_TYPE Gun_GetDefaultType(void);
// Whether the weapon throws an explosive that flies on its own.
bool Gun_IsLauncherType(LARA_GUN_TYPE gun_type);
// Whether the weapon sends a projectile that flies, rather than a round that
// arrives where Lara aims at once.
bool Gun_FiresProjectile(LARA_GUN_TYPE gun_type);
// Returns the weapon a projectile was sent by, and LGT_UNARMED where the
// object is no weapon's.
LARA_GUN_TYPE Gun_GetTypeForProjectile(OBJECT_ID obj_id);
// Returns the projectile the weapon sends, and NO_OBJECT where it sends a
// round that arrives at once instead.
OBJECT_ID Gun_GetProjectileObject(LARA_GUN_TYPE gun_type);
bool Gun_IsFlareType(LARA_GUN_TYPE gun_type);
// Returns the gun type Lara carries a flare as, and LGT_UNARMED where no
// weapon is one.
LARA_GUN_TYPE Gun_GetFlareType(void);
bool Gun_IsSinglePistolType(LARA_GUN_TYPE gun_type);
// Whether Lara holds one of the weapon in each hand.
bool Gun_IsDualPistolType(LARA_GUN_TYPE gun_type);

void Gun_AddDynamicLight(void);
void Gun_FireOverlaySound(const WEAPON_INFO *weapon);
