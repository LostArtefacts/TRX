#pragma once

#include <trx/game/lara/types.h>
#include <trx/game/objects/ids.h>

#include <lualib.h>
#include <stdint.h>

// How many pickups may be declared as sharing another's backpack entry.
#define FAKE_INV_SHARED 4

// The backpack holds this many distinct objects, which is plenty for a test.
#define FAKE_INV_SLOTS 16

// A weapon's pickup and its clips. Real ids, as fakes/items.c holds real
// pickups: the bridge checks what it is handed against the object table, so a
// made-up id is rejected before it reaches the backpack.
OBJECT_ID FakeLara_GunObject(LARA_GUN_TYPE gun_type);
OBJECT_ID FakeLara_AmmoObject(LARA_GUN_TYPE gun_type);

// Whether the level carries the inventory models at all. False makes every add
// refuse, the way a level that never loaded the icon does.
void FakeLara_SetCanAdd(bool can_add);
void FakeLara_SetWeaponAvailable(LARA_GUN_TYPE gun_type, bool available);

// Declares that a pickup goes into another's backpack entry, the way the
// scion's carried state goes into the scion's.
void FakeLara_ShareInvEntry(OBJECT_ID variant, OBJECT_ID base);
