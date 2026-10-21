#include <trx/game/inventory.h>

#include <trx/config.h>
#include <trx/core/log.h>
#include <trx/core/utils.h>
#include <trx/game/game.h>
#include <trx/game/gun.h>
#include <trx/game/gun/common.h>
#include <trx/game/gun/registry.h>
#include <trx/game/lara.h>
#include <trx/game/objects/combos.h>
#include <trx/game/objects/families.h>
#include <trx/game/objects/links.h>
#include <trx/game/objects/names.h>

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
    // A weapon that goes takes its attachment with it.
    if (gun_type != LGT_UNARMED && qty <= 0) {
        Inv_State_SetAttachment(state, gun_type, false);
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
    if (!ObjectFamily_Has(pickup_id, OBJ_FAMILY_AMMO)) {
        return LGT_UNARMED;
    }
    return Gun_GetType(ObjectLink_GetInverse(pickup_id, OBJ_LINK_GUN_TO_AMMO));
}

// How much one add is worth, which for a box of flares is the shots in it.
static int32_t M_GetAddQty(const OBJECT_ID object_id)
{
    return object_id == O_FLARES_BOX_ITEM
        ? Gun_Registry_Get(Gun_GetFlareType())->ammo.box_shots
        : 1;
}

// The kind of ammunition a box entry is, which is the weapon's first kind for
// a box the weapon does not list.
static OBJECT_ID M_GetAmmoKindOf(
    const OBJECT_ID object_id, const LARA_GUN_TYPE gun_type)
{
    const OBJECT_ID pickup_id = Inv_GetItemPickup(object_id);
    return Gun_IsAmmoKind(gun_type, pickup_id) ? pickup_id
                                               : Gun_GetAmmoKind(gun_type, 0);
}

// How many boxes the rounds of one kind come to, which is what a box entry
// counts. Nothing stores that count: the rounds are the whole of it.
static int32_t M_GetAmmoBoxCount(
    const INVENTORY_STATE *const state, const LARA_GUN_TYPE gun_type,
    const OBJECT_ID kind)
{
    return Inv_State_GetKindAmmo(state, gun_type, kind)
        / Gun_GetRoundsPerBox(gun_type);
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

// Return the stored rounds for one kind of a weapon's ammunition, or nullptr
// if the inventory has no entry for it.
static INVENTORY_AMMO *M_FindAmmoSlot(
    INVENTORY_STATE *const state, const LARA_GUN_TYPE owner,
    const OBJECT_ID kind)
{
    for (int32_t i = 0; i < state->ammo_count; i++) {
        if (state->ammo[i].gun_type == owner && state->ammo[i].kind == kind) {
            return &state->ammo[i];
        }
    }
    return nullptr;
}

static INVENTORY_AMMO *M_AddAmmoSlot(
    INVENTORY_STATE *const state, const LARA_GUN_TYPE owner,
    const OBJECT_ID kind)
{
    INVENTORY_AMMO *slot = M_FindAmmoSlot(state, owner, kind);
    if (slot != nullptr) {
        return slot;
    }
    if (state->ammo_count >= (int32_t)ARRAY_SIZE(state->ammo)) {
        LOG_WARNING(
            "no room left to carry rounds for weapon %d", (int32_t)owner);
        return nullptr;
    }
    slot = &state->ammo[state->ammo_count++];
    *slot = (INVENTORY_AMMO) { .gun_type = owner, .kind = kind };
    return slot;
}

static OBJECT_ID M_GetSelectedKind(
    const INVENTORY_STATE *const state, const LARA_GUN_TYPE owner)
{
    for (int32_t i = 0; i < state->ammo_count; i++) {
        const INVENTORY_AMMO *const slot = &state->ammo[i];
        if (slot->gun_type == owner && slot->is_selected
            && Gun_IsAmmoKind(owner, slot->kind)) {
            return slot->kind;
        }
    }
    return Gun_GetAmmoKind(owner, 0);
}

static void M_AddKindAmmo(
    const LARA_GUN_TYPE gun_type, const OBJECT_ID kind, const int32_t rounds)
{
    Inv_SetKindAmmo(gun_type, kind, Inv_GetKindAmmo(gun_type, kind) + rounds);
}

// The weapon an entry draws, and LGT_UNARMED for any other entry.
static LARA_GUN_TYPE M_GetEntryGunType(const OBJECT_ID entry_id)
{
    return Gun_GetType(Inv_GetItemPickup(entry_id));
}

// The entry of the attachment a weapon takes, or NO_OBJECT for a weapon that
// takes none.
static OBJECT_ID M_GetAttachmentEntry(const LARA_GUN_TYPE gun_type)
{
    if (gun_type == LGT_UNARMED || !Gun_Registry_IsValidType(gun_type)) {
        return NO_OBJECT;
    }
    const OBJECT_ID object_id =
        Gun_Registry_Get(gun_type)->attachment.object_id;
    return object_id == NO_OBJECT ? NO_OBJECT : M_GetEntryID(object_id);
}

// Whether a weapon of the inventory carries the attachment an entry is.
static bool M_IsAttachmentCarried(
    const INVENTORY_STATE *const state, const OBJECT_ID entry_id)
{
    for (int32_t i = 0; i < state->attached_count; i++) {
        if (M_GetAttachmentEntry(state->attached[i]) == entry_id) {
            return true;
        }
    }
    return false;
}

static bool M_IsAttachment(const OBJECT_ID entry_id)
{
    for (int32_t i = 0; i < Gun_Registry_GetCount(); i++) {
        const LARA_GUN_TYPE gun_type = Gun_Registry_GetByIndex(i)->gun_type;
        if (M_GetAttachmentEntry(gun_type) == entry_id) {
            return true;
        }
    }
    return false;
}

// Whether a weapon and an attachment of the inventory can be put together.
static bool M_CanAttach(
    const LARA_GUN_TYPE gun_type, const OBJECT_ID attachment_entry_id)
{
    return M_GetAttachmentEntry(gun_type) == attachment_entry_id
        && Inv_HasItem(Gun_GetGunObject(gun_type))
        && Inv_HasItem(attachment_entry_id) && !Inv_HasAttachment(gun_type);
}

// Draws the weapon in Lara's hand again where she holds it, as it changes
// with its attachment.
static void M_RefreshHeldWeapon(const LARA_GUN_TYPE gun_type)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->gun_type == gun_type && lara->gun_status != LGS_ARMLESS
        && lara->gun_status != LGS_HANDS_BUSY) {
        Gun_SetLaraHandRMesh(gun_type);
    }
}

bool Inv_HasAmmoSlot(const LARA_GUN_TYPE gun_type)
{
    return M_GetAmmoOwner(gun_type) != LGT_UNARMED;
}

int32_t Inv_State_GetKindAmmo(
    const INVENTORY_STATE *const state, const LARA_GUN_TYPE gun_type,
    const OBJECT_ID kind)
{
    const LARA_GUN_TYPE owner = M_GetAmmoOwner(gun_type);
    if (owner == LGT_UNARMED) {
        return 0;
    }
    const INVENTORY_AMMO *const slot =
        M_FindAmmoSlot((INVENTORY_STATE *)state, owner, kind);
    return slot == nullptr ? 0 : slot->rounds;
}

void Inv_State_SetKindAmmo(
    INVENTORY_STATE *const state, const LARA_GUN_TYPE gun_type,
    const OBJECT_ID kind, const int32_t rounds)
{
    const LARA_GUN_TYPE owner = M_GetAmmoOwner(gun_type);
    if (owner == LGT_UNARMED) {
        return;
    }
    INVENTORY_AMMO *const slot = M_AddAmmoSlot(state, owner, kind);
    if (slot != nullptr) {
        slot->rounds = MAX(0, MIN(rounds, MAX_QTY));
    }
}

void Inv_State_FillAmmo(
    INVENTORY_STATE *const state, const LARA_GUN_TYPE gun_type,
    const int32_t rounds)
{
    const int32_t count = Gun_GetAmmoKindCount(gun_type);
    if (count == 0) {
        Inv_State_SetAmmo(state, gun_type, rounds);
        return;
    }
    for (int32_t i = 0; i < count; i++) {
        Inv_State_SetKindAmmo(
            state, gun_type, Gun_GetAmmoKind(gun_type, i), rounds);
    }
}

OBJECT_ID Inv_State_GetAmmoKind(
    const INVENTORY_STATE *const state, const LARA_GUN_TYPE gun_type)
{
    const LARA_GUN_TYPE owner = M_GetAmmoOwner(gun_type);
    return owner == LGT_UNARMED ? NO_OBJECT : M_GetSelectedKind(state, owner);
}

void Inv_State_SelectAmmoKind(
    INVENTORY_STATE *const state, const LARA_GUN_TYPE gun_type,
    const OBJECT_ID kind)
{
    const LARA_GUN_TYPE owner = M_GetAmmoOwner(gun_type);
    if (owner == LGT_UNARMED || !Gun_IsAmmoKind(owner, kind)
        || M_AddAmmoSlot(state, owner, kind) == nullptr) {
        return;
    }
    for (int32_t i = 0; i < state->ammo_count; i++) {
        INVENTORY_AMMO *const slot = &state->ammo[i];
        if (slot->gun_type == owner) {
            slot->is_selected = slot->kind == kind;
        }
    }
}

int32_t Inv_State_GetAmmo(
    const INVENTORY_STATE *const state, const LARA_GUN_TYPE gun_type)
{
    return Inv_State_GetKindAmmo(
        state, gun_type, Inv_State_GetAmmoKind(state, gun_type));
}

void Inv_State_SetAmmo(
    INVENTORY_STATE *const state, const LARA_GUN_TYPE gun_type,
    const int32_t rounds)
{
    Inv_State_SetKindAmmo(
        state, gun_type, Inv_State_GetAmmoKind(state, gun_type), rounds);
}

void Inv_State_ClearAmmo(INVENTORY_STATE *const state)
{
    state->ammo_count = 0;
}

bool Inv_State_HasAttachment(
    const INVENTORY_STATE *const state, const LARA_GUN_TYPE gun_type)
{
    for (int32_t i = 0; i < state->attached_count; i++) {
        if (state->attached[i] == gun_type) {
            return true;
        }
    }
    return false;
}

void Inv_State_SetAttachment(
    INVENTORY_STATE *const state, const LARA_GUN_TYPE gun_type,
    const bool attached)
{
    for (int32_t i = 0; i < state->attached_count; i++) {
        if (state->attached[i] == gun_type) {
            if (!attached) {
                state->attached[i] = state->attached[--state->attached_count];
            }
            return;
        }
    }
    if (attached
        && state->attached_count < (int32_t)ARRAY_SIZE(state->attached)) {
        state->attached[state->attached_count++] = gun_type;
    }
}

void Inv_State_ClearAttachments(INVENTORY_STATE *const state)
{
    state->attached_count = 0;
}

void Inv_State_CopyAttachments(
    INVENTORY_STATE *const dst, const INVENTORY_STATE *const src)
{
    memcpy(dst->attached, src->attached, sizeof(dst->attached));
    dst->attached_count = src->attached_count;
}

bool Inv_HasAttachment(const LARA_GUN_TYPE gun_type)
{
    return Inv_State_HasAttachment(&m_State, gun_type);
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
    Inv_State_SetAmmo(&m_State, gun_type, rounds);
}

void Inv_AddAmmo(const LARA_GUN_TYPE gun_type, const int32_t rounds)
{
    Inv_SetAmmo(gun_type, Inv_GetAmmo(gun_type) + rounds);
}

int32_t Inv_GetKindAmmo(const LARA_GUN_TYPE gun_type, const OBJECT_ID kind)
{
    return Inv_State_GetKindAmmo(&m_State, gun_type, kind);
}

void Inv_FillAmmo(const LARA_GUN_TYPE gun_type, const int32_t rounds)
{
    Inv_State_FillAmmo(&m_State, gun_type, rounds);
}

void Inv_SetKindAmmo(
    const LARA_GUN_TYPE gun_type, const OBJECT_ID kind, const int32_t rounds)
{
    Inv_State_SetKindAmmo(&m_State, gun_type, kind, rounds);
}

OBJECT_ID Inv_GetAmmoKind(const LARA_GUN_TYPE gun_type)
{
    return Inv_State_GetAmmoKind(&m_State, gun_type);
}

void Inv_SelectAmmoKind(const LARA_GUN_TYPE gun_type, const OBJECT_ID kind)
{
    Inv_State_SelectAmmoKind(&m_State, gun_type, kind);
}

int32_t Inv_State_GetCount(
    const INVENTORY_STATE *const state, const OBJECT_ID object_id)
{
    const OBJECT_ID entry_id = M_GetEntryID(object_id);
    const LARA_GUN_TYPE gun_type = M_GetAmmoGunType(entry_id);
    if (gun_type != LGT_UNARMED) {
        return M_GetAmmoBoxCount(
            state, gun_type, M_GetAmmoKindOf(entry_id, gun_type));
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
        Inv_State_SetKindAmmo(
            state, gun_type, M_GetAmmoKindOf(entry_id, gun_type),
            qty * Gun_GetRoundsPerBox(gun_type));
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
}

OBJECT_ID Inv_GetItemOption(const OBJECT_ID object_id)
{
    if (ObjectFamily_Has(object_id, OBJ_FAMILY_INVENTORY)) {
        return object_id;
    }
    return ObjectLink_Get(object_id, OBJ_LINK_ITEM_TO_OPTION);
}

OBJECT_ID Inv_GetItemPickup(const OBJECT_ID object_id)
{
    if (ObjectFamily_Has(object_id, OBJ_FAMILY_INVENTORY)) {
        const OBJECT_ID pickup_id =
            ObjectLink_GetInverse(object_id, OBJ_LINK_ITEM_TO_OPTION);
        // Use the ring object itself when it has no separate pickup.
        return pickup_id == NO_OBJECT ? object_id : pickup_id;
    }
    return object_id;
}

int32_t Inv_GetItemCount(const OBJECT_ID object_id)
{
    return Inv_State_GetCount(&m_State, object_id);
}

int32_t Inv_GetCombinePartners(
    const OBJECT_ID object_id, OBJECT_ID *const partners,
    const int32_t max_count)
{
    const OBJECT_ID entry_id = M_GetEntryID(object_id);
    const int32_t total = ObjectCombo_GetPartnerCount(entry_id);
    int32_t count = 0;
    for (int32_t i = 0; i < total && count < max_count; i++) {
        const OBJECT_ID partner = ObjectCombo_GetPartnerAt(entry_id, i);
        if (Inv_HasItem(partner)) {
            partners[count++] = partner;
        }
    }

    // An attachment combines with every weapon that takes it, and a weapon
    // with the attachment it takes.
    const LARA_GUN_TYPE gun_type = M_GetEntryGunType(entry_id);
    if (gun_type != LGT_UNARMED) {
        if (count < max_count
            && M_CanAttach(gun_type, M_GetAttachmentEntry(gun_type))) {
            partners[count++] = M_GetAttachmentEntry(gun_type);
        }
        return count;
    }
    for (int32_t i = 0; i < Gun_Registry_GetCount() && count < max_count; i++) {
        const LARA_GUN_TYPE other = Gun_Registry_GetByIndex(i)->gun_type;
        if (M_CanAttach(other, entry_id)) {
            partners[count++] = M_GetEntryID(Gun_GetGunObject(other));
        }
    }
    return count;
}

OBJECT_ID Inv_Combine(const OBJECT_ID object_id, const OBJECT_ID partner_id)
{
    if (object_id == partner_id || !Inv_HasItem(object_id)
        || !Inv_HasItem(partner_id)) {
        return NO_OBJECT;
    }

    const OBJECT_ID entry_id = M_GetEntryID(object_id);
    const OBJECT_ID partner_entry_id = M_GetEntryID(partner_id);
    LARA_GUN_TYPE gun_type = M_GetEntryGunType(entry_id);
    OBJECT_ID attachment_id = partner_entry_id;
    if (gun_type == LGT_UNARMED) {
        gun_type = M_GetEntryGunType(partner_entry_id);
        attachment_id = entry_id;
    }
    if (gun_type != LGT_UNARMED) {
        if (!M_CanAttach(gun_type, attachment_id)) {
            return NO_OBJECT;
        }
        Inv_RemoveItem(attachment_id);
        Inv_State_SetAttachment(&m_State, gun_type, true);
        M_RefreshHeldWeapon(gun_type);
        return M_GetEntryID(Gun_GetGunObject(gun_type));
    }
    const OBJECT_ID result = ObjectCombo_GetResult(object_id, partner_id);
    if (result == NO_OBJECT) {
        return NO_OBJECT;
    }
    Inv_RemoveItem(object_id);
    Inv_RemoveItem(partner_id);
    Inv_AddItem(result);
    return result;
}

OBJECT_ID Inv_Separate(const OBJECT_ID object_id)
{
    const LARA_GUN_TYPE gun_type = M_GetEntryGunType(M_GetEntryID(object_id));
    if (gun_type == LGT_UNARMED || !Inv_HasAttachment(gun_type)) {
        return NO_OBJECT;
    }
    const OBJECT_ID attachment_id = M_GetAttachmentEntry(gun_type);
    Inv_State_SetAttachment(&m_State, gun_type, false);
    M_SetCount(&m_State, attachment_id, Inv_GetItemCount(attachment_id) + 1);
    M_RefreshHeldWeapon(gun_type);
    return attachment_id;
}

INV_ITEM_ACTIONS Inv_GetItemActions(const OBJECT_ID object_id)
{
    const OBJECT_ID entry_id = M_GetEntryID(object_id);
    const LARA_GUN_TYPE gun_type = M_GetEntryGunType(entry_id);
    // Picking a weapon draws it, so a weapon combines with its attachment
    // only where a menu offers that apart from picking it.
    OBJECT_ID partner;
    const bool can_combine = gun_type == LGT_UNARMED
        && Inv_GetCombinePartners(entry_id, &partner, 1) > 0;
    // The binoculars are a generic entry that Lara uses by herself rather
    // than on a lock or a vehicle.
    const bool leads_nowhere = entry_id != O_BINOCULARS_OPTION
        && ObjectFamily_Has(entry_id, OBJ_FAMILY_GENERIC_INV_OPTION)
        && ObjectLink_Get(entry_id, OBJ_LINK_KEY_TO_RECEPTACLE) == NO_OBJECT
        && ObjectLink_Get(entry_id, OBJ_LINK_KEY_TO_VEHICLE) == NO_OBJECT;
    return (INV_ITEM_ACTIONS) {
        .can_use = !leads_nowhere && !can_combine,
        .can_examine = g_Config.gameplay.enable_item_examining
            && Object_GetDescription(entry_id) != nullptr,
        .can_combine = can_combine,
        .can_separate = gun_type != LGT_UNARMED && Inv_HasAttachment(gun_type),
    };
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
    // A box of each kind of ammunition is drawn for rounds she has no gun to
    // spend, and stops being drawn the moment she finds one.
    for (int32_t i = 0; i < Gun_Registry_GetCount() && count < max_count; i++) {
        const LARA_GUN_TYPE gun_type = Gun_Registry_GetByIndex(i)->gun_type;
        if (Inv_State_Has(state, Gun_GetGunObject(gun_type))) {
            continue;
        }
        const int32_t kind_count = Gun_GetAmmoKindCount(gun_type);
        for (int32_t j = 0; j < kind_count && count < max_count; j++) {
            const OBJECT_ID kind = Gun_GetAmmoKind(gun_type, j);
            const int32_t qty = M_GetAmmoBoxCount(state, gun_type, kind);
            if (qty > 0) {
                entries[count++] = (INVENTORY_ENTRY) {
                    .object_id = M_GetEntryID(kind),
                    .qty = qty,
                };
            }
        }
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
}

bool Inv_AddItemNTimes(const OBJECT_ID object_id, const int32_t qty)
{
    if (qty <= 0 || !Inv_AddItem(object_id)) {
        return false;
    }

    // The first one carried every effect an add has, and the rest only raise
    // the number, so they are applied together rather than one at a time.
    const int32_t rest = MIN(qty, MAX_QTY) - 1;
    if (rest == 0 || Inv_GetItemOption(object_id) == O_BINOCULARS_OPTION) {
        return true;
    }

    const OBJECT_ID entry_id = M_GetEntryID(object_id);
    const LARA_GUN_TYPE ammo_gun_type = M_GetAmmoGunType(entry_id);
    if (ammo_gun_type != LGT_UNARMED) {
        M_AddKindAmmo(
            ammo_gun_type, M_GetAmmoKindOf(entry_id, ammo_gun_type),
            Gun_GetRoundsPerBox(ammo_gun_type) * M_GetAddQty(object_id) * rest);
        return true;
    }

    const INVENTORY_ENTRY *const entry = M_FindEntry(&m_State, entry_id);
    if (entry != nullptr) {
        M_SetCount(
            &m_State, entry_id, entry->qty + M_GetAddQty(object_id) * rest);
    }
    return true;
}

void Inv_ClearItem(const OBJECT_ID object_id)
{
    if (!Inv_RemoveItem(object_id)) {
        return;
    }

    const OBJECT_ID entry_id = M_GetEntryID(object_id);
    const LARA_GUN_TYPE gun_type = M_GetAmmoGunType(entry_id);
    if (gun_type != LGT_UNARMED) {
        const OBJECT_ID kind = M_GetAmmoKindOf(entry_id, gun_type);
        const int32_t per_box = Gun_GetRoundsPerBox(gun_type);
        Inv_SetKindAmmo(
            gun_type, kind, Inv_GetKindAmmo(gun_type, kind) % per_box);
        return;
    }
    M_SetCount(&m_State, entry_id, 0);
}

bool Inv_RemoveItem(const OBJECT_ID object_id)
{
    const OBJECT_ID entry_id = M_GetEntryID(object_id);
    const LARA_GUN_TYPE gun_type = M_GetAmmoGunType(entry_id);
    const INVENTORY_ENTRY *const entry = M_FindEntry(&m_State, entry_id);
    if (gun_type == LGT_UNARMED && entry == nullptr) {
        return false;
    }
    const OBJECT_ID kind = gun_type == LGT_UNARMED
        ? NO_OBJECT
        : M_GetAmmoKindOf(entry_id, gun_type);
    if (gun_type != LGT_UNARMED
        && M_GetAmmoBoxCount(&m_State, gun_type, kind) <= 0) {
        return false;
    }

    if (gun_type != LGT_UNARMED) {
        // A box is worth its rounds and no more. What is left over once the
        // last whole one is gone draws nothing until she finds more, but it
        // is hers all the same.
        M_AddKindAmmo(gun_type, kind, -Gun_GetRoundsPerBox(gun_type));
    } else {
        M_SetCount(&m_State, entry_id, entry->qty - 1);
    }
    return true;
}

void Inv_Clear(void)
{
    Inv_SetState(&(INVENTORY_STATE) {});
}

// What Inv_AddItem needs before it can take anything: the level has to carry
// the inventory model, which is not the same as carrying the pickup. A level
// with no shotgun lying in it still draws one in the ring.
bool Inv_CanAddItem(const OBJECT_ID object_id)
{
    const OBJECT_ID inv_object_id = Inv_GetItemOption(object_id);
    const OBJECT_ID wanted =
        inv_object_id == NO_OBJECT ? object_id : inv_object_id;
    if (wanted == NO_OBJECT) {
        return false;
    }
    return Object_Get(wanted)->loaded;
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

    // There is only one of an attachment, whether loose or on a weapon.
    const OBJECT_ID attachment_entry_id = M_GetEntryID(object_id);
    if (M_IsAttachment(attachment_entry_id)
        && (Inv_HasItem(attachment_entry_id)
            || M_IsAttachmentCarried(&m_State, attachment_entry_id))) {
        return false;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (ObjectFamily_Has(pickup_object_id, OBJ_FAMILY_GUN)) {
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

    const int32_t qty = M_GetAddQty(object_id);
    const OBJECT_ID entry_id = M_GetEntryID(object_id);

    // Every box of ammunition goes the same way: rounds of its own kind, and
    // nothing stored.
    const LARA_GUN_TYPE ammo_gun_type = M_GetAmmoGunType(entry_id);
    if (ammo_gun_type != LGT_UNARMED) {
        M_AddKindAmmo(
            ammo_gun_type, M_GetAmmoKindOf(entry_id, ammo_gun_type),
            Gun_GetRoundsPerBox(ammo_gun_type) * qty);
        return true;
    }

    const INVENTORY_ENTRY *const entry = M_FindEntry(&m_State, entry_id);
    if (entry != nullptr) {
        M_SetCount(&m_State, entry_id, entry->qty + qty);
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
        return true;
    }

    // Other guns
    if (ObjectFamily_Has(pickup_object_id, OBJ_FAMILY_GUN)) {
        M_AddGun(Gun_GetType(pickup_object_id));
        return true;
    }

    // Other cases
    if (ObjectFamily_Has(entry_id, OBJ_FAMILY_INVENTORY)) {
        M_SetCount(&m_State, entry_id, qty);
        return true;
    }
    return false;
}
