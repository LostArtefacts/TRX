#pragma once

#include <trx/core/result.h>
#include <trx/core/utils.h>
#include <trx/game/gun/types.h>
#include <trx/game/lara/enum.h>

#include <stdint.h>

// Clear the weapon table before reading the weapon file.
void Gun_Registry_Seed(void);

// Apply the routines for an implemented weapon kind.
// Report failure if the kind has no implementation.
RESULT Gun_Registry_SetKind(WEAPON_TYPE type, WEAPON_INFO *target);

// Declare a weapon with the routines and save keys for an implemented kind.
// Report failure if the kind has no implementation or the weapon is already
// declared.
RESULT Gun_Registry_Declare(
    LARA_GUN_TYPE gun_type, WEAPON_TYPE type, WEAPON_INFO *target);

// Returns the weapon a gun type stands for. Every valid type has one, and a
// type nothing implements reads as empty, which is the case for empty hands
// and for a gun fixed to a vehicle. Read is_declared to tell the two apart.
WEAPON_INFO *Gun_Registry_Get(LARA_GUN_TYPE gun_type);

// Returns the weapon at an index, counting from zero, in the order the gun
// types are numbered, and passing over the types nothing implements.
const WEAPON_INFO *Gun_Registry_GetByIndex(int32_t idx);

// The number of weapons the engine implements, which is fewer than the gun
// types it knows.
int32_t Gun_Registry_GetCount(void);

// Give a weapon the key that draws it, taking that key from whichever
// weapon holds it now. One key draws one weapon, so a mod that claims the
// shotgun's key draws its own weapon with it.
void Gun_Registry_SetInputRole(LARA_GUN_TYPE gun_type, INPUT_ROLE role);

// Keep a string for the rest of the process and return the stored copy.
const char *Gun_Registry_KeepString(const char *str);

// Return whether the catalog contains the gun type. Legacy saves may contain
// unsupported types.
bool Gun_Registry_IsValidType(LARA_GUN_TYPE gun_type);
