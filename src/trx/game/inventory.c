#include <trx/game/inventory.h>

#include <trx/core/log.h>
#include <trx/game/game.h>
#include <trx/game/gun.h>
#include <trx/game/inventory_ring.h>
#include <trx/game/lara.h>
#include <trx/game/objects/vars.h>

static INVENTORY_STATE m_State = {};

// The entry a pickup goes into. An object with no icon of its own stands for
// itself, which is how the stopwatch and the compass are addressed.
static OBJECT_ID M_GetEntryID(const OBJECT_ID object_id)
{
    const OBJECT_ID option_id = Inv_GetItemOption(object_id);
    return option_id == NO_OBJECT ? object_id : option_id;
}

static INVENTORY_ENTRY *M_FindEntry(const OBJECT_ID object_id)
{
    for (int32_t i = 0; i < m_State.count; i++) {
        if (m_State.entries[i].object_id == object_id) {
            return &m_State.entries[i];
        }
    }
    return nullptr;
}

// Writes what Lara has of one thing, without touching the rings: every caller
// here rebuilds them once it is done.
static void M_SetCount(const OBJECT_ID object_id, const int32_t qty)
{
    INVENTORY_ENTRY *const entry = M_FindEntry(object_id);
    if (qty <= 0) {
        if (entry != nullptr) {
            const int32_t idx = entry - m_State.entries;
            m_State.count--;
            for (int32_t i = idx; i < m_State.count; i++) {
                m_State.entries[i] = m_State.entries[i + 1];
            }
        }
        return;
    }

    if (entry != nullptr) {
        entry->qty = MIN(qty, MAX_QTY);
        return;
    }
    if (m_State.count >= INV_MAX_ENTRIES) {
        LOG_WARNING("no room in the inventory for object %d", object_id);
        return;
    }
    m_State.entries[m_State.count++] = (INVENTORY_ENTRY) {
        .object_id = object_id,
        .qty = MIN(qty, MAX_QTY),
    };
}

static void M_AddGun(const LARA_GUN_TYPE gun_type)
{
    const OBJECT_ID gun_object = Gun_GetGunObject(gun_type);
    const OBJECT_ID ammo_object = Gun_GetAmmoObject(gun_type);
    LARA_INFO *const lara = Lara_GetLaraInfo();
    InvRing_NotifyRemoved(M_GetEntryID(ammo_object));
    M_SetCount(M_GetEntryID(ammo_object), 0);
    Inv_AddAmmo(gun_type, Gun_GetInitialRounds(gun_type));
    M_SetCount(M_GetEntryID(gun_object), 1);
    if (lara->last_gun_type == LGT_UNARMED) {
        lara->last_gun_type = gun_type;
    }
    Item_GlobalReplace(gun_object, ammo_object);
}

static void M_AddAmmo(const LARA_GUN_TYPE gun_type)
{
    const OBJECT_ID gun_object = Gun_GetGunObject(gun_type);
    Inv_AddAmmo(gun_type, Gun_GetRoundsPerBox(gun_type));
    if (!Inv_HasItem(gun_object)) {
        M_SetCount(M_GetEntryID(Gun_GetAmmoObject(gun_type)), 1);
    }
}

// Where a weapon's rounds are kept, or nullptr for one that spends none. The
// skidoo shoots from the pistols' endless supply.
static int32_t *M_GetAmmoSlot(const LARA_GUN_TYPE gun_type)
{
    if (gun_type == LGT_SKIDOO) {
        return &m_State.ammo[LGT_PISTOLS];
    }
    if (gun_type <= LGT_UNARMED || gun_type >= NUM_WEAPONS
        || gun_type == LGT_FLARE) {
        return nullptr;
    }
    return &m_State.ammo[gun_type];
}

bool Inv_HasAmmoSlot(const LARA_GUN_TYPE gun_type)
{
    return M_GetAmmoSlot(gun_type) != nullptr;
}

int32_t Inv_GetAmmo(const LARA_GUN_TYPE gun_type)
{
    const int32_t *const slot = M_GetAmmoSlot(gun_type);
    return slot == nullptr ? 0 : *slot;
}

void Inv_SetAmmo(const LARA_GUN_TYPE gun_type, const int32_t rounds)
{
    int32_t *const slot = M_GetAmmoSlot(gun_type);
    if (slot != nullptr) {
        *slot = MIN(rounds, MAX_QTY);
    }
}

void Inv_AddAmmo(const LARA_GUN_TYPE gun_type, const int32_t rounds)
{
    int32_t *const slot = M_GetAmmoSlot(gun_type);
    if (slot != nullptr) {
        *slot += rounds;
        CLAMPG(*slot, MAX_QTY);
    }
}

INVENTORY_STATE *Inv_GetState(void)
{
    return &m_State;
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
    const INVENTORY_ENTRY *const entry = M_FindEntry(M_GetEntryID(object_id));
    return entry == nullptr ? 0 : entry->qty;
}

bool Inv_HasItem(const OBJECT_ID object_id)
{
    return Inv_GetItemCount(object_id) > 0;
}

void Inv_SetItemCount(const OBJECT_ID object_id, const int32_t qty)
{
    M_SetCount(M_GetEntryID(object_id), qty);
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
    const INVENTORY_ENTRY *const entry = M_FindEntry(entry_id);
    if (entry == nullptr) {
        return false;
    }

    // While the rings still hold it, so that the cursor can be moved off the
    // position that is about to close up.
    InvRing_NotifyRemoved(entry_id);
    M_SetCount(entry_id, entry->qty - 1);
    InvRing_Rebuild();
    return true;
}

void Inv_RemoveAllItems(void)
{
    m_State.count = 0;

    Inv_AddItem(O_STOPWATCH_OPTION);
    Inv_AddItem(O_COMPASS_OPTION);
    Inv_AddItem(O_GLOBE_SELECT_OPTION);

    InvRing_Rebuild();
    InvRing_ClearSelection();
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

    const int32_t qty =
        object_id == O_FLAREBOX_ITEM ? g_Weapons[LGT_FLARE].ammo.box_shots : 1;
    const OBJECT_ID entry_id = M_GetEntryID(object_id);
    const INVENTORY_ENTRY *const entry = M_FindEntry(entry_id);
    if (entry != nullptr) {
        if (Object_IsType(pickup_object_id, g_GunAmmoObjects)) {
            const LARA_GUN_TYPE gun_type = Gun_GetType(
                Object_GetCognateInverse(pickup_object_id, g_GunAmmoObjectMap));
            Inv_AddAmmo(gun_type, Gun_GetRoundsPerBox(gun_type));
        }
        M_SetCount(entry_id, entry->qty + qty);
        InvRing_Rebuild();
        return true;
    }

    // Pistols
    if (inv_object_id == O_PISTOL_OPTION) {
        M_SetCount(O_PISTOL_OPTION, 1);
        if (lara->last_gun_type == LGT_UNARMED) {
            lara->last_gun_type = LGT_PISTOLS;
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
    if (Object_IsType(pickup_object_id, g_GunAmmoObjects)) {
        M_AddAmmo(Gun_GetType(
            Object_GetCognateInverse(pickup_object_id, g_GunAmmoObjectMap)));
        InvRing_Rebuild();
        return true;
    }

    // Other cases
    if (InvRing_GetByObjectID(entry_id) != nullptr) {
        M_SetCount(entry_id, qty);
        InvRing_Rebuild();
        return true;
    }
    return false;
}
