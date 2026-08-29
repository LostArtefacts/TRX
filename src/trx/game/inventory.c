#include <trx/game/inventory.h>

#include <trx/core/log.h>
#include <trx/core/utils.h>
#include <trx/game/game.h>
#include <trx/game/gun.h>
#include <trx/game/gun/common.h>
#include <trx/game/gun/registry.h>
#include <trx/game/inventory_ring.h>
#include <trx/game/lara.h>
#include <trx/game/objects/vars.h>

#include <string.h>

static INVENTORY_STATE m_State = {};

// The entry a pickup goes into. An object with no icon of its own stands for
// itself, which is how the stopwatch and the compass are addressed.
static OBJECT_ID M_GetEntryID(const OBJECT_ID object_id)
{
    const OBJECT_ID option_id = Inv_GetItemOption(object_id);
    return option_id == NO_OBJECT ? object_id : option_id;
}

// Where a thing sits in the state, and -1 for one she is not carrying.
static int32_t M_FindEntryIndex(
    const INVENTORY_STATE *const state, const OBJECT_ID object_id)
{
    for (int32_t i = 0; i < state->count; i++) {
        if (state->entries[i].object_id == object_id) {
            return i;
        }
    }
    return -1;
}

static const INVENTORY_ENTRY *M_FindEntry(
    const INVENTORY_STATE *const state, const OBJECT_ID object_id)
{
    const int32_t idx = M_FindEntryIndex(state, object_id);
    return idx < 0 ? nullptr : &state->entries[idx];
}

// Writes what Lara has of one thing, without touching the rings: every caller
// here rebuilds them once it is done.
static void M_SetCount(
    INVENTORY_STATE *const state, const OBJECT_ID object_id, const int32_t qty)
{
    // While an endless supply never runs out, the number behind it stands
    // for the gun rather than for anything she picked up. Left behind, it
    // would draw boxes of clips she never found.
    const LARA_GUN_TYPE gun_type = Gun_GetType(Inv_GetItemPickup(object_id));
    if (gun_type != LGT_UNARMED && Gun_HasInfiniteAmmo(gun_type)) {
        Inv_State_SetAmmo(
            state, gun_type, qty > 0 ? Gun_GetInitialRounds(gun_type) : 0);
    }

    const int32_t idx = M_FindEntryIndex(state, object_id);
    if (qty <= 0) {
        if (idx >= 0) {
            state->count--;
            for (int32_t i = idx; i < state->count; i++) {
                state->entries[i] = state->entries[i + 1];
            }
        }
        return;
    }

    if (idx >= 0) {
        state->entries[idx].qty = MIN(qty, MAX_QTY);
        return;
    }
    if (state->count >= INV_MAX_ENTRIES) {
        LOG_WARNING("no room in the inventory for object %d", object_id);
        return;
    }
    state->entries[state->count++] = (INVENTORY_ENTRY) {
        .object_id = object_id,
        .qty = MIN(qty, MAX_QTY),
    };
}

static void M_AddGun(const LARA_GUN_TYPE gun_type)
{
    const OBJECT_ID gun_object = Gun_GetGunObject(gun_type);
    const OBJECT_ID ammo_object = Gun_GetAmmoObject(gun_type);
    LARA_INFO *const lara = Lara_GetLaraInfo();
    // The boxes she was carrying stop being drawn of their own accord: the
    // rounds in them are hers either way, and now she has the gun to spend
    // them from.
    Inv_AddAmmo(gun_type, Gun_GetInitialRounds(gun_type));
    M_SetCount(&m_State, M_GetEntryID(gun_object), 1);
    if (lara->last_gun_type == LGT_UNARMED) {
        lara->last_gun_type = gun_type;
    }
    Item_GlobalReplace(gun_object, ammo_object);
}

// The weapon a box of ammunition belongs to, and LGT_UNARMED for an entry
// that is not one.
static LARA_GUN_TYPE M_GetAmmoGunType(const OBJECT_ID object_id)
{
    const OBJECT_ID pickup_id = Inv_GetItemPickup(object_id);
    if (!Object_IsType(pickup_id, g_GunAmmoObjects)) {
        return LGT_UNARMED;
    }
    return Gun_GetType(Object_GetCognateInverse(pickup_id, g_GunAmmoObjectMap));
}

// How many boxes the rounds come to, which is what a box entry counts. Nothing
// stores that count: the rounds are the whole of it.
static int32_t M_GetAmmoBoxCount(
    const INVENTORY_STATE *const state, const LARA_GUN_TYPE gun_type)
{
    return Inv_State_GetAmmo(state, gun_type) / Gun_GetRoundsPerBox(gun_type);
}

// Identify the weapon whose rounds are spent, or LGT_UNARMED if none are
// spent; vehicle-mounted weapons use the default weapon's unlimited supply.
static LARA_GUN_TYPE M_GetAmmoOwner(const LARA_GUN_TYPE gun_type)
{
    if (Gun_Registry_IsValidType(gun_type)
        && Gun_Registry_Get(gun_type)->type == WEAPON_TYPE_MOUNTED) {
        return Gun_GetDefaultType();
    }
    if (gun_type <= LGT_UNARMED || !Gun_Registry_IsValidType(gun_type)
        || Gun_IsFlareType(gun_type)) {
        return LGT_UNARMED;
    }
    return gun_type;
}

// Return the stored rounds for a weapon, or nullptr if the inventory has no
// entry for it.
static int32_t *M_FindAmmoSlot(
    INVENTORY_STATE *const state, const LARA_GUN_TYPE owner)
{
    for (int32_t i = 0; i < state->ammo_count; i++) {
        if (state->ammo[i].gun_type == owner) {
            return &state->ammo[i].rounds;
        }
    }
    return nullptr;
}

bool Inv_HasAmmoSlot(const LARA_GUN_TYPE gun_type)
{
    return M_GetAmmoOwner(gun_type) != LGT_UNARMED;
}

int32_t Inv_State_GetAmmo(
    const INVENTORY_STATE *const state, const LARA_GUN_TYPE gun_type)
{
    const LARA_GUN_TYPE owner = M_GetAmmoOwner(gun_type);
    if (owner == LGT_UNARMED) {
        return 0;
    }
    const int32_t *const slot = M_FindAmmoSlot((INVENTORY_STATE *)state, owner);
    return slot == nullptr ? 0 : *slot;
}

void Inv_State_SetAmmo(
    INVENTORY_STATE *const state, const LARA_GUN_TYPE gun_type,
    const int32_t rounds)
{
    const LARA_GUN_TYPE owner = M_GetAmmoOwner(gun_type);
    if (owner == LGT_UNARMED) {
        return;
    }
    int32_t *slot = M_FindAmmoSlot(state, owner);
    if (slot == nullptr) {
        if (state->ammo_count >= (int32_t)ARRAY_SIZE(state->ammo)) {
            LOG_WARNING(
                "no room left to carry rounds for weapon %d", (int32_t)owner);
            return;
        }
        state->ammo[state->ammo_count].gun_type = owner;
        slot = &state->ammo[state->ammo_count].rounds;
        state->ammo_count++;
    }
    *slot = MAX(0, MIN(rounds, MAX_QTY));
}

void Inv_State_ClearAmmo(INVENTORY_STATE *const state)
{
    state->ammo_count = 0;
}

void Inv_State_CopyAmmo(
    INVENTORY_STATE *const dst, const INVENTORY_STATE *const src)
{
    memcpy(dst->ammo, src->ammo, sizeof(dst->ammo));
    dst->ammo_count = src->ammo_count;
}

int32_t Inv_GetAmmo(const LARA_GUN_TYPE gun_type)
{
    return Inv_State_GetAmmo(&m_State, gun_type);
}

void Inv_SetAmmo(const LARA_GUN_TYPE gun_type, const int32_t rounds)
{
    // A box entry counts nothing of its own, so the rings have to be redrawn
    // whenever the rounds behind it come to a different number of boxes.
    const int32_t old_box_count = M_GetAmmoBoxCount(&m_State, gun_type);
    Inv_State_SetAmmo(&m_State, gun_type, rounds);
    if (M_GetAmmoBoxCount(&m_State, gun_type) != old_box_count) {
        InvRing_Rebuild();
    }
}

void Inv_AddAmmo(const LARA_GUN_TYPE gun_type, const int32_t rounds)
{
    Inv_SetAmmo(gun_type, Inv_GetAmmo(gun_type) + rounds);
}

int32_t Inv_State_GetCount(
    const INVENTORY_STATE *const state, const OBJECT_ID object_id)
{
    const OBJECT_ID entry_id = M_GetEntryID(object_id);
    const LARA_GUN_TYPE gun_type = M_GetAmmoGunType(entry_id);
    if (gun_type != LGT_UNARMED) {
        return M_GetAmmoBoxCount(state, gun_type);
    }
    const INVENTORY_ENTRY *const entry = M_FindEntry(state, entry_id);
    return entry == nullptr ? 0 : entry->qty;
}

bool Inv_State_Has(
    const INVENTORY_STATE *const state, const OBJECT_ID object_id)
{
    return Inv_State_GetCount(state, object_id) > 0;
}

void Inv_State_SetCount(
    INVENTORY_STATE *const state, const OBJECT_ID object_id, const int32_t qty)
{
    const OBJECT_ID entry_id = M_GetEntryID(object_id);
    const LARA_GUN_TYPE gun_type = M_GetAmmoGunType(entry_id);
    if (gun_type != LGT_UNARMED) {
        Inv_State_SetAmmo(state, gun_type, qty * Gun_GetRoundsPerBox(gun_type));
        return;
    }
    M_SetCount(state, entry_id, qty);
}

void Inv_State_AddAmmo(
    INVENTORY_STATE *const state, const LARA_GUN_TYPE gun_type,
    const int32_t rounds)
{
    Inv_State_SetAmmo(
        state, gun_type, Inv_State_GetAmmo(state, gun_type) + rounds);
}

void Inv_State_AddCount(
    INVENTORY_STATE *const state, const OBJECT_ID object_id, const int32_t qty)
{
    Inv_State_SetCount(
        state, object_id, Inv_State_GetCount(state, object_id) + qty);
}

INVENTORY_STATE *Inv_GetState(void)
{
    return &m_State;
}

void Inv_EnsureItem(const OBJECT_ID object_id)
{
    if (!Inv_HasItem(object_id)) {
        Inv_AddItem(object_id);
    }
}

void Inv_SetState(const INVENTORY_STATE *const state)
{
    m_State = *state;
    // A level that has not loaded the model has nothing to draw the entry
    // with, and what the rings cannot show she is not given to carry.
    m_State.count = 0;
    for (int32_t i = 0; i < state->count; i++) {
        if (Inv_CanAddItem(state->entries[i].object_id)) {
            m_State.entries[m_State.count++] = state->entries[i];
        }
    }
    // The compass and the stopwatch are not carried so much as always to hand,
    // so they are put back regardless of what the state says.
    Inv_EnsureItem(O_STOPWATCH_OPTION);
    Inv_EnsureItem(O_COMPASS_OPTION);
    Inv_EnsureItem(O_GLOBE_OPTION);
    InvRing_Rebuild();
}

OBJECT_ID Inv_GetItemOption(const OBJECT_ID object_id)
{
    if (Object_IsType(object_id, g_InvObjects)) {
        return object_id;
    }
    return Object_GetCognate(object_id, g_ItemToInvObjectMap);
}

OBJECT_ID Inv_GetItemPickup(const OBJECT_ID object_id)
{
    if (Object_IsType(object_id, g_InvObjects)) {
        return Object_GetCognateInverse(object_id, g_ItemToInvObjectMap);
    }
    return object_id;
}

int32_t Inv_GetItemCount(const OBJECT_ID object_id)
{
    return Inv_State_GetCount(&m_State, object_id);
}

int32_t Inv_State_GetDrawnEntries(
    const INVENTORY_STATE *const state, INVENTORY_ENTRY *const entries,
    const int32_t max_count)
{
    int32_t count = 0;
    for (int32_t i = 0; i < state->count && count < max_count; i++) {
        if (M_GetAmmoGunType(state->entries[i].object_id) == LGT_UNARMED) {
            entries[count++] = state->entries[i];
        }
    }
    // A box of ammunition is drawn for rounds she has no gun to spend, and
    // stops being drawn the moment she finds one.
    for (int32_t i = 0; i < Gun_Registry_GetCount() && count < max_count; i++) {
        const LARA_GUN_TYPE gun_type = Gun_Registry_GetByIndex(i)->gun_type;
        const OBJECT_ID ammo_object = Gun_GetAmmoObject(gun_type);
        if (ammo_object == NO_OBJECT
            || Inv_State_Has(state, Gun_GetGunObject(gun_type))
            || M_GetAmmoBoxCount(state, gun_type) <= 0) {
            continue;
        }
        entries[count++] = (INVENTORY_ENTRY) {
            .object_id = M_GetEntryID(ammo_object),
            .qty = M_GetAmmoBoxCount(state, gun_type),
        };
    }
    return count;
}

int32_t Inv_GetDrawnEntries(
    INVENTORY_ENTRY *const entries, const int32_t max_count)
{
    return Inv_State_GetDrawnEntries(&m_State, entries, max_count);
}

bool Inv_HasItem(const OBJECT_ID object_id)
{
    return Inv_GetItemCount(object_id) > 0;
}

void Inv_SetItemCount(const OBJECT_ID object_id, const int32_t qty)
{
    Inv_State_SetCount(&m_State, object_id, qty);
    InvRing_Rebuild();
}

bool Inv_AddItemNTimes(const OBJECT_ID object_id, const int32_t qty)
{
    bool result = false;
    for (int32_t i = 0; i < qty; i++) {
        result |= Inv_AddItem(object_id);
    }
    return result;
}

bool Inv_RemoveItem(const OBJECT_ID object_id)
{
    const OBJECT_ID entry_id = M_GetEntryID(object_id);
    const LARA_GUN_TYPE gun_type = M_GetAmmoGunType(entry_id);
    const INVENTORY_ENTRY *const entry = M_FindEntry(&m_State, entry_id);
    if (gun_type == LGT_UNARMED && entry == nullptr) {
        return false;
    }
    if (gun_type != LGT_UNARMED && M_GetAmmoBoxCount(&m_State, gun_type) <= 0) {
        return false;
    }

    // While the rings still hold it, so that the cursor can be moved off the
    // position that is about to close up.
    InvRing_NotifyRemoved(entry_id);
    if (gun_type != LGT_UNARMED) {
        // A box is worth its rounds and no more. What is left over once the
        // last whole one is gone draws nothing until she finds more, but it
        // is hers all the same.
        Inv_SetAmmo(
            gun_type, Inv_GetAmmo(gun_type) - Gun_GetRoundsPerBox(gun_type));
    } else {
        M_SetCount(&m_State, entry_id, entry->qty - 1);
    }
    InvRing_Rebuild();
    return true;
}

void Inv_RemoveAllItems(void)
{
    Inv_SetState(&(INVENTORY_STATE) {});
    InvRing_ClearSelection();
    InvRing_ForgetLastEntries();
}

// What Inv_AddItem needs before it can take anything: the level has to carry
// the inventory model, which is not the same as carrying the pickup. A level
// with no shotgun lying in it still draws one in the ring.
bool Inv_CanAddItem(const OBJECT_ID object_id)
{
    const OBJECT_ID inv_object_id = Inv_GetItemOption(object_id);
    const OBJECT *const object =
        Object_Get(inv_object_id == NO_OBJECT ? object_id : inv_object_id);
    return object->loaded;
}

bool Inv_AddItem(const OBJECT_ID object_id)
{
    const OBJECT_ID inv_object_id = Inv_GetItemOption(object_id);
    const OBJECT_ID pickup_object_id = Inv_GetItemPickup(object_id);
    if (!Inv_CanAddItem(object_id)) {
        return false;
    }

    if (inv_object_id == O_BINOCULARS_OPTION
        && Inv_HasItem(O_BINOCULARS_ITEM)) {
        return false;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (Object_IsType(pickup_object_id, g_GunObjects)) {
        Gun_UpdateLaraMeshes(pickup_object_id);
        if (lara->gun_type == LGT_UNARMED) {
            lara->gun_type = Gun_GetType(pickup_object_id);
            const bool hands_busy = lara->gun_status == LGS_HANDS_BUSY;
            lara->gun_status = LGS_ARMLESS;
            Gun_InitialiseNewWeapon();
            if (hands_busy) {
                lara->gun_status = LGS_HANDS_BUSY;
            }
        }
    }

    const int32_t qty = object_id == O_FLARES_BOX_ITEM
        ? Gun_Registry_Get(Gun_GetFlareType())->ammo.box_shots
        : 1;
    const OBJECT_ID entry_id = M_GetEntryID(object_id);

    // Every spelling of a box of ammunition goes the same way, including the
    // variants that share one icon with it: rounds, and nothing stored.
    const LARA_GUN_TYPE ammo_gun_type = M_GetAmmoGunType(entry_id);
    if (ammo_gun_type != LGT_UNARMED) {
        Inv_AddAmmo(ammo_gun_type, Gun_GetRoundsPerBox(ammo_gun_type) * qty);
        InvRing_Rebuild();
        return true;
    }

    const INVENTORY_ENTRY *const entry = M_FindEntry(&m_State, entry_id);
    if (entry != nullptr) {
        M_SetCount(&m_State, entry_id, entry->qty + qty);
        InvRing_Rebuild();
        return true;
    }

    // The default weapon arrives loaded, as every other weapon arrives with
    // the rounds it is picked up with. It is kept out of Item_GlobalReplace:
    // a level that is not meant to hold it says so through the game flow.
    const LARA_GUN_TYPE default_gun = Gun_GetDefaultType();
    if (Gun_GetType(Inv_GetItemPickup(inv_object_id)) == default_gun
        && default_gun != LGT_UNARMED) {
        Inv_AddAmmo(default_gun, Gun_GetInitialRounds(default_gun));
        M_SetCount(&m_State, inv_object_id, 1);
        if (lara->last_gun_type == LGT_UNARMED) {
            lara->last_gun_type = default_gun;
        }
        InvRing_Rebuild();
        return true;
    }

    // Other guns
    if (Object_IsType(pickup_object_id, g_GunObjects)) {
        M_AddGun(Gun_GetType(pickup_object_id));
        InvRing_Rebuild();
        return true;
    }

    // Other cases
    if (InvRing_GetByObjectID(entry_id) != nullptr) {
        M_SetCount(&m_State, entry_id, qty);
        InvRing_Rebuild();
        return true;
    }
    return false;
}
