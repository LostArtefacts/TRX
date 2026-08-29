#pragma once

#include <trx/game/gun/const.h>
#include <trx/game/lara/enum.h>
#include <trx/game/objects/ids.h>

#include <stdint.h>

// How many kinds of thing Lara can carry at once. The rings can draw fewer
// than this, so a level runs out of somewhere to show them first.
#define INV_MAX_ENTRIES 100

typedef struct {
    // The inventory option, which is what the rings draw. Several pickups can
    // share one, so this is what Inv_GetItemOption hands back rather than the
    // object lying in the level.
    OBJECT_ID object_id;
    int32_t qty;
} INVENTORY_ENTRY;

typedef struct {
    LARA_GUN_TYPE gun_type;
    int32_t rounds;
} INVENTORY_AMMO;

// What Lara is carrying. Holding no pointers, it is copied by assignment,
// which is how a level hands what she has to the next one.
typedef struct {
    INVENTORY_ENTRY entries[INV_MAX_ENTRIES];
    int32_t count;
    // Store rounds by weapon identity so catalog-created weapons can have
    // ammunition even when Lara does not carry the corresponding weapon.
    INVENTORY_AMMO ammo[MAX_WEAPONS];
    int32_t ammo_count;
} INVENTORY_STATE;

// What Lara is carrying, which the rings are drawn from. Writing it puts her
// in front of a whole inventory at once, as arriving in a level does.
INVENTORY_STATE *Inv_GetState(void);
void Inv_SetState(const INVENTORY_STATE *state);

// Reading and writing an inventory Lara is not carrying: the one a level
// keeps for when she arrives in it.
int32_t Inv_State_GetCount(const INVENTORY_STATE *state, OBJECT_ID obj_id);
bool Inv_State_Has(const INVENTORY_STATE *state, OBJECT_ID obj_id);
void Inv_State_SetCount(INVENTORY_STATE *state, OBJECT_ID obj_id, int32_t qty);
void Inv_State_AddCount(INVENTORY_STATE *state, OBJECT_ID obj_id, int32_t qty);
void Inv_State_AddAmmo(
    INVENTORY_STATE *state, LARA_GUN_TYPE gun_type, int32_t rounds);
int32_t Inv_State_GetAmmo(const INVENTORY_STATE *state, LARA_GUN_TYPE gun_type);
void Inv_State_SetAmmo(
    INVENTORY_STATE *state, LARA_GUN_TYPE gun_type, int32_t rounds);
// Remove all rounds from the inventory when a level supplies Lara's
// ammunition itself.
void Inv_State_ClearAmmo(INVENTORY_STATE *state);
void Inv_State_CopyAmmo(INVENTORY_STATE *dst, const INVENTORY_STATE *src);
int32_t Inv_State_GetDrawnEntries(
    const INVENTORY_STATE *state, INVENTORY_ENTRY *entries, int32_t max_count);

OBJECT_ID Inv_GetItemOption(OBJECT_ID obj_id);
OBJECT_ID Inv_GetItemPickup(OBJECT_ID obj_id);

bool Inv_CanAddItem(OBJECT_ID obj_id);
int32_t Inv_GetItemCount(OBJECT_ID obj_id);
// What the rings draw: what Lara carries, and the boxes of ammunition standing
// for rounds she has no gun to spend. Answers how many were written.
int32_t Inv_GetDrawnEntries(INVENTORY_ENTRY *entries, int32_t max_count);
bool Inv_HasItem(OBJECT_ID obj_id);
void Inv_SetItemCount(OBJECT_ID obj_id, int32_t qty);
bool Inv_AddItem(OBJECT_ID obj_id);
// Puts one of something in, and no more than one however often it is asked
// for.
void Inv_EnsureItem(OBJECT_ID obj_id);
bool Inv_AddItemNTimes(OBJECT_ID obj_id, int32_t qty);
bool Inv_RemoveItem(OBJECT_ID obj_id);
void Inv_RemoveAllItems(void);

// Whether the weapon spends rounds at all, which the flare and the unarmed
// hand do not.
bool Inv_HasAmmoSlot(LARA_GUN_TYPE gun_type);
// Rounds for one weapon. What a round is worth is the weapon's business, and
// Gun_GetRoundsPerShot and Gun_GetRoundsPerBox answer for it.
int32_t Inv_GetAmmo(LARA_GUN_TYPE gun_type);
void Inv_SetAmmo(LARA_GUN_TYPE gun_type, int32_t rounds);
void Inv_AddAmmo(LARA_GUN_TYPE gun_type, int32_t rounds);
