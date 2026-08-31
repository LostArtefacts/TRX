#include <trx/game/game_flow/inventory.h>

#include <trx/config.h>
#include <trx/game/catalog/table.h>
#include <trx/game/gun.h>
#include <trx/game/gun/common.h>
#include <trx/game/gun/registry.h>
#include <trx/game/inventory.h>
#include <trx/game/objects.h>
#include <trx/game/objects/families.h>
#include <trx/game/overlay.h>
#include <trx/game/savegame.h>
#include <trx/game/stats.h>

#include <string.h>

CATALOG_TABLE_DEFINE(m_SecretInvItems, CATALOG_OBJECTS, int8_t);
CATALOG_TABLE_DEFINE(m_Add2InvItems, CATALOG_OBJECTS, int8_t);
static bool m_RemoveWeapons = false;
static bool m_RemoveAmmo = false;
static bool m_RemoveFlares = false;
static bool m_RemoveMedipacks = false;
static bool m_RemoveScions = false;
static bool m_RemoveBinoculars = false;

// What the scion modifier takes away. The object ids are not consecutive, so
// they are named one by one.
static const OBJECT_ID m_ScionObjects[] = {
    O_SCION_ITEM_1, O_QUEST_ITEM_1, O_QUEST_ITEM_2,
    O_QUEST_ITEM_3, O_QUEST_ITEM_4, NO_OBJECT,
};

static int8_t *M_SecretInvItem(const OBJECT_ID object_id)
{
    return CatalogTable_Get(&m_SecretInvItems, object_id);
}

static int8_t *M_Add2InvItem(const OBJECT_ID object_id)
{
    return CatalogTable_Get(&m_Add2InvItems, object_id);
}

static bool M_CanHaveItem(const OBJECT_ID object_id)
{
    if (ObjectFamily_Has(object_id, OBJ_FAMILY_GUN)
        && object_id != O_PISTOLS_ITEM
        && g_Config.gameplay.disable_extra_guns) {
        return false;
    }
    if ((object_id == O_SMALL_MEDIPACK_ITEM
         || object_id == O_LARGE_MEDIPACK_ITEM)
        && g_Config.gameplay.disable_medpacks) {
        return false;
    }
    return true;
}

static void M_ResumeInfo_AddItem(
    RESUME_INFO *const resume, const OBJECT_ID object_id, const int32_t qty)
{
    // The binoculars are held rather than counted, so any number of them is
    // the one pair.
    if (object_id == O_BINOCULARS_ITEM) {
        if (qty > 0) {
            Inv_State_SetCount(&resume->inv, object_id, 1);
        }
        return;
    }
    Inv_State_AddCount(&resume->inv, object_id, qty);
}

static void M_ModifyResumeInfo_GunOrAmmo(
    RESUME_INFO *const resume, const LARA_GUN_TYPE gun_type)
{
    const OBJECT_ID gun_object_id = Gun_GetGunObject(gun_type);
    const OBJECT_ID ammo_object_id = Gun_GetAmmoObject(gun_type);
    const int32_t ammo_pickup_qty = Gun_GetRoundsPerBox(gun_type);
    const int32_t ammo_initial_qty = Gun_GetInitialRounds(gun_type);

    if (!M_CanHaveItem(gun_object_id) || !M_CanHaveItem(ammo_object_id)) {
        return;
    }

    Inv_State_AddAmmo(
        &resume->inv, gun_type,
        ammo_pickup_qty * *M_Add2InvItem(ammo_object_id));
    if (!Inv_State_Has(&resume->inv, gun_object_id)
        && *M_Add2InvItem(gun_object_id) > 0) {
        Inv_State_SetCount(&resume->inv, gun_object_id, 1);
        Inv_State_AddAmmo(&resume->inv, gun_type, ammo_initial_qty);
    }
}

static void M_ModifyResumeInfo_Item(
    RESUME_INFO *const resume, const OBJECT_ID object_id)
{
    if (!M_CanHaveItem(object_id)) {
        return;
    }

    M_ResumeInfo_AddItem(resume, object_id, *M_Add2InvItem(object_id));
}

static void M_CollectNewPickup(const OBJECT_ID object_id)
{
    Overlay_AddDisplayPickup(object_id);
    Stats_AddPickup();
}

static void M_ModifyInventory_GunOrAmmo(
    const GF_INV_TYPE type, const LARA_GUN_TYPE gun_type)
{
    const OBJECT_ID gun_object_id = Gun_GetGunObject(gun_type);
    const OBJECT_ID ammo_object_id = Gun_GetAmmoObject(gun_type);
    const int32_t ammo_pickup_qty = Gun_GetRoundsPerBox(gun_type);
    const int32_t ammo_initial_qty = Gun_GetInitialRounds(gun_type);

    if (!M_CanHaveItem(gun_object_id) || !M_CanHaveItem(ammo_object_id)) {
        return;
    }

    if (Inv_HasItem(gun_object_id)) {
        if (type == GF_INV_SECRET) {
            // Convert already collected guns into ammo to maintain stats
            // accuracy.
            Inv_AddAmmo(
                gun_type, ammo_pickup_qty * *M_SecretInvItem(ammo_object_id));
            Inv_AddAmmo(
                gun_type, ammo_initial_qty * *M_SecretInvItem(gun_object_id));
            for (int32_t i = 0; i < *M_SecretInvItem(ammo_object_id); i++) {
                M_CollectNewPickup(ammo_object_id);
            }
            for (int32_t i = 0; i < *M_SecretInvItem(gun_object_id); i++) {
                M_CollectNewPickup(ammo_object_id);
            }
        } else if (type == GF_INV_REGULAR) {
            Inv_AddAmmo(
                gun_type, ammo_pickup_qty * *M_Add2InvItem(ammo_object_id));
        }
    } else if (
        (type == GF_INV_REGULAR && *M_Add2InvItem(gun_object_id) > 0)
        || (type == GF_INV_SECRET && *M_SecretInvItem(gun_object_id) > 0)) {

        Inv_AddItem(gun_object_id);

        if (type == GF_INV_SECRET) {
            Inv_AddAmmo(
                gun_type, ammo_pickup_qty * *M_SecretInvItem(ammo_object_id));
            M_CollectNewPickup(gun_object_id);
            for (int32_t i = 0; i < *M_SecretInvItem(ammo_object_id); i++) {
                M_CollectNewPickup(ammo_object_id);
            }
        } else if (type == GF_INV_REGULAR) {
            Inv_AddAmmo(
                gun_type, ammo_pickup_qty * *M_Add2InvItem(ammo_object_id));
        }
    } else if (type == GF_INV_SECRET) {
        for (int32_t i = 0; i < *M_SecretInvItem(ammo_object_id); i++) {
            Inv_AddItem(ammo_object_id);
            M_CollectNewPickup(ammo_object_id);
        }
    } else if (type == GF_INV_REGULAR) {
        for (int32_t i = 0; i < *M_Add2InvItem(ammo_object_id); i++) {
            Inv_AddItem(ammo_object_id);
        }
    }
}

static void M_ModifyInventory_Item(
    const GF_INV_TYPE type, const OBJECT_ID object_id)
{
    int32_t qty = 0;
    if (type == GF_INV_SECRET) {
        qty = *M_SecretInvItem(object_id);
    } else if (type == GF_INV_REGULAR) {
        qty = *M_Add2InvItem(object_id);
    }

    // Check for gameplay mods from secret rewards
    if (!M_CanHaveItem(object_id)) {
        qty = 0;
    }

    for (int32_t i = 0; i < qty; i++) {
        if (Inv_AddItem(object_id) && type == GF_INV_SECRET) {
            M_CollectNewPickup(object_id);
        }
    }
}

void GF_InventoryModifier_Scan(const GF_LEVEL *const level)
{
    CATALOG_FOR_EACH(CATALOG_OBJECTS, i)
    {
        *M_SecretInvItem(i) = 0;
        *M_Add2InvItem(i) = 0;
    }
    m_RemoveWeapons = false;
    m_RemoveAmmo = false;
    m_RemoveFlares = false;
    m_RemoveMedipacks = false;
    m_RemoveScions = false;
    m_RemoveBinoculars = false;

    if (level == nullptr) {
        return;
    }
    for (int32_t i = 0; i < level->sequence.length; i++) {
        const GF_SEQUENCE_EVENT *const event = &level->sequence.events[i];
        if (event->type == GFS_ADD_ITEM
            || event->type == GFS_ADD_SECRET_REWARD) {
            const GF_ADD_ITEM_DATA *const data = event->data;
            if (!Catalog_IsValidID(CATALOG_OBJECTS, data->object_id)) {
                continue;
            }
            if (data->inv_type == GF_INV_SECRET) {
                *M_SecretInvItem(data->object_id) += data->quantity;
            } else if (data->inv_type == GF_INV_REGULAR) {
                *M_Add2InvItem(data->object_id) += data->quantity;
            }
        } else if (event->type == GFS_REMOVE_WEAPONS) {
            m_RemoveWeapons = true;
        } else if (event->type == GFS_REMOVE_AMMO) {
            m_RemoveAmmo = true;
        } else if (event->type == GFS_REMOVE_FLARES) {
            m_RemoveFlares = true;
        } else if (event->type == GFS_REMOVE_MEDIPACKS) {
            m_RemoveMedipacks = true;
        } else if (event->type == GFS_REMOVE_SCIONS) {
            m_RemoveScions = true;
        } else if (event->type == GFS_REMOVE_BINOCULARS) {
            m_RemoveBinoculars = true;
        }
    }
}

int32_t GF_GetSecretRewardCount(const GF_LEVEL *const level)
{
    int32_t sum = 0;
    if (level == nullptr) {
        return sum;
    }
    for (int32_t i = 0; i < level->sequence.length; i++) {
        const GF_SEQUENCE_EVENT *const event = &level->sequence.events[i];
        if (event->type == GFS_ADD_SECRET_REWARD) {
            const GF_ADD_ITEM_DATA *const data = event->data;
            if (data->inv_type == GF_INV_SECRET) {
                sum += data->quantity;
            }
        }
    }
    return sum;
}

void GF_InventoryModifier_ApplyToResumeInfo(const GF_LEVEL *const level)
{
    RESUME_INFO *const resume = SG_Resume_GetEntry(level);

    if (m_RemoveWeapons) {
        for (int32_t i = 0; i < Gun_Registry_GetCount(); i++) {
            const LARA_GUN_TYPE gun_type = Gun_Registry_GetByIndex(i)->gun_type;
            Inv_State_SetCount(&resume->inv, Gun_GetGunObject(gun_type), 0);
        }
        resume->holsters_gun_type = LGT_UNARMED;
        resume->back_gun_type = LGT_UNARMED;
        resume->equipped_gun_type = LGT_UNARMED;
        resume->gun_status = LGS_ARMLESS;
    }

    const LARA_GUN_TYPE default_gun = Gun_GetDefaultType();
    const OBJECT_ID default_gun_object = Gun_GetGunObject(default_gun);
    const bool default_gun_given =
        !Inv_State_Has(&resume->inv, default_gun_object)
        && *M_Add2InvItem(default_gun_object) != 0;
    if (default_gun_given) {
        Inv_State_SetCount(&resume->inv, default_gun_object, 1);
        if (resume->equipped_gun_type == LGT_UNARMED) {
            resume->equipped_gun_type = default_gun;
        }
    }

    if (m_RemoveAmmo) {
        Inv_State_ClearAmmo(&resume->inv);
    }

    // The game flow always uses the loaded gun, even if the level took the
    // rest away. The count is updated in Inv_State_SetCount.
    if (default_gun_given && !Gun_HasInfiniteAmmo(default_gun)) {
        Inv_State_AddAmmo(
            &resume->inv, default_gun, Gun_GetInitialRounds(default_gun));
    }

    if (m_RemoveScions) {
        for (int32_t i = 0; m_ScionObjects[i] != NO_OBJECT; i++) {
            Inv_State_SetCount(&resume->inv, m_ScionObjects[i], 0);
        }
        m_RemoveScions = false;
    }

    if (m_RemoveFlares) {
        Inv_State_SetCount(&resume->inv, O_FLARE_ITEM, 0);
        m_RemoveFlares = false;
    }

    if (m_RemoveMedipacks) {
        Inv_State_SetCount(&resume->inv, O_LARGE_MEDIPACK_ITEM, 0);
        Inv_State_SetCount(&resume->inv, O_SMALL_MEDIPACK_ITEM, 0);
        m_RemoveMedipacks = false;
    }

    if (m_RemoveBinoculars) {
        Inv_State_SetCount(&resume->inv, O_BINOCULARS_ITEM, 0);
    } else {
        M_ModifyResumeInfo_Item(resume, O_BINOCULARS_ITEM);
    }

    for (int32_t i = 0; i < Gun_Registry_GetCount(); i++) {
        const LARA_GUN_TYPE gun_type = Gun_Registry_GetByIndex(i)->gun_type;
        if (Gun_GetGunObject(gun_type) != NO_OBJECT) {
            M_ModifyResumeInfo_GunOrAmmo(resume, gun_type);
        }
    }

    M_ModifyResumeInfo_Item(resume, O_SMALL_MEDIPACK_ITEM);
    M_ModifyResumeInfo_Item(resume, O_LARGE_MEDIPACK_ITEM);
    M_ModifyResumeInfo_Item(resume, O_FLARE_ITEM);
}

void GF_InventoryModifier_Apply(
    const GF_LEVEL *const level, const GF_INV_TYPE type)
{
    RESUME_INFO *const resume = SG_Resume_GetEntry(level);

    // For GF_INV_REGULAR, we must ignore weapons, ammo, medpacks and flares,
    // as these are handled by RESUME_INFO and
    // GF_InventoryModifier_ApplyToResumeInfo and Lara_InitialiseInventory.

    if (type == GF_INV_SECRET) {
        const LARA_GUN_TYPE default_gun = Gun_GetDefaultType();
        if (*M_Add2InvItem(Gun_GetGunObject(default_gun))) {
            Inv_AddItem(Gun_GetGunObject(default_gun));
            if (resume->equipped_gun_type == LGT_UNARMED) {
                resume->equipped_gun_type = default_gun;
            }
        }

        // The default weapon arrives without ammunition.
        for (int32_t i = 0; i < Gun_Registry_GetCount(); i++) {
            const LARA_GUN_TYPE gun_type = Gun_Registry_GetByIndex(i)->gun_type;
            if (gun_type != default_gun
                && Gun_GetGunObject(gun_type) != NO_OBJECT) {
                M_ModifyInventory_GunOrAmmo(type, gun_type);
            }
        }
    }

    // Return every pickup reached by the rules in catalogue order, preserving
    // the scion's position by handling it after the walk.
    CATALOG_FOR_EACH(CATALOG_OBJECTS, object_id)
    {
        if (object_id == O_SCION_ITEM_1
            || !ObjectFamily_Has(object_id, OBJ_FAMILY_PICKUP)
            || ObjectFamily_Has(object_id, OBJ_FAMILY_GUN)
            || ObjectFamily_Has(object_id, OBJ_FAMILY_AMMO)
            || ObjectFamily_Has(object_id, OBJ_FAMILY_SUPPLY)
            || ObjectFamily_Has(object_id, OBJ_FAMILY_SECRET)) {
            continue;
        }
        M_ModifyInventory_Item(type, object_id);
    }
    M_ModifyInventory_Item(type, O_SCION_ITEM_1);

    if (type == GF_INV_SECRET) {
        M_ModifyInventory_Item(type, O_SMALL_MEDIPACK_ITEM);
        M_ModifyInventory_Item(type, O_LARGE_MEDIPACK_ITEM);
        M_ModifyInventory_Item(type, O_FLARE_ITEM);
    }
}
