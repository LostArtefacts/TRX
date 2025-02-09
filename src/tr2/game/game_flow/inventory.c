#include "game/game_flow/inventory.h"

#include "decomp/savegame.h"
#include "game/gun/gun.h"
#include "game/inventory.h"
#include "game/overlay.h"
#include "global/vars.h"

static int8_t m_SecretInvItems[O_NUMBER_OF] = {};
static int8_t m_Add2InvItems[O_NUMBER_OF] = {};
static bool m_RemoveWeapons = false;
static bool m_RemoveAmmo = false;
static bool m_RemoveFlares = false;
static bool m_RemoveMedipacks = false;

static void M_ModifyInventory_GunOrAmmo(
    START_INFO *start, GF_INV_TYPE type, LARA_GUN_TYPE gun_type);
static void M_ModifyInventory_Item(GF_INV_TYPE type, GAME_OBJECT_ID obj_id);

static void M_ModifyInventory_GunOrAmmo(
    START_INFO *const start, const GF_INV_TYPE type,
    const LARA_GUN_TYPE gun_type)
{
    const GAME_OBJECT_ID gun_item = Gun_GetGunObject(gun_type);
    const GAME_OBJECT_ID ammo_item = Gun_GetAmmoObject(gun_type);
    const int32_t ammo_qty = Gun_GetAmmoQuantity(gun_type);
    AMMO_INFO *const ammo_info = Gun_GetAmmoInfo(gun_type);

    if (Inv_RequestItem(gun_item)) {
        if (type == GF_INV_SECRET) {
            ammo_info->ammo += ammo_qty * m_SecretInvItems[ammo_item];
            for (int32_t i = 0; i < m_SecretInvItems[ammo_item]; i++) {
                Overlay_AddDisplayPickup(ammo_item);
            }
        } else if (type == GF_INV_REGULAR) {
            ammo_info->ammo += ammo_qty * m_Add2InvItems[ammo_item];
        }
    } else if (
        (type == GF_INV_REGULAR && m_Add2InvItems[gun_item])
        || (type == GF_INV_SECRET && m_SecretInvItems[gun_item])) {

        // clang-format off
        // TODO: consider moving this to Inv_AddItem
        switch (gun_type) {
        case LGT_PISTOLS: start->has_pistols = 1; break;
        case LGT_MAGNUMS: start->has_magnums = 1; break;
        case LGT_UZIS:    start->has_uzis = 1;    break;
        case LGT_SHOTGUN: start->has_shotgun = 1; break;
        case LGT_HARPOON: start->has_harpoon = 1; break;
        case LGT_M16:     start->has_m16 = 1;     break;
        case LGT_GRENADE: start->has_grenade = 1; break;
        default: break;
        }
        // clang-format on

        Inv_AddItem(gun_item);

        if (type == GF_INV_SECRET) {
            ammo_info->ammo += ammo_qty * m_SecretInvItems[ammo_item];
            Overlay_AddDisplayPickup(gun_item);
            for (int32_t i = 0; i < m_SecretInvItems[ammo_item]; i++) {
                Overlay_AddDisplayPickup(ammo_item);
            }
        } else if (type == GF_INV_REGULAR) {
            ammo_info->ammo += ammo_qty * m_Add2InvItems[ammo_item];
        }
    } else if (type == GF_INV_SECRET) {
        for (int32_t i = 0; i < m_SecretInvItems[ammo_item]; i++) {
            Inv_AddItem(ammo_item);
            Overlay_AddDisplayPickup(ammo_item);
        }
    } else if (type == GF_INV_REGULAR) {
        for (int32_t i = 0; i < m_Add2InvItems[ammo_item]; i++) {
            Inv_AddItem(ammo_item);
        }
    }
}

static void M_ModifyInventory_Item(
    const GF_INV_TYPE type, const GAME_OBJECT_ID obj_id)
{
    int32_t qty = 0;
    if (type == GF_INV_SECRET) {
        qty = m_SecretInvItems[obj_id];
    } else if (type == GF_INV_REGULAR) {
        qty = m_Add2InvItems[obj_id];
    }

    for (int32_t i = 0; i < qty; i++) {
        Inv_AddItem(obj_id);
        if (type == GF_INV_SECRET) {
            Overlay_AddDisplayPickup(obj_id);
        }
    }
}

void GF_InventoryModifier_Scan(const GF_LEVEL *const level)
{
    for (int32_t i = 0; i < O_NUMBER_OF; i++) {
        m_SecretInvItems[i] = 0;
        m_Add2InvItems[i] = 0;
    }
    m_RemoveWeapons = false;
    m_RemoveAmmo = false;
    m_RemoveFlares = false;
    m_RemoveMedipacks = false;

    if (level == nullptr) {
        return;
    }
    for (int32_t i = 0; i < level->sequence.length; i++) {
        const GF_SEQUENCE_EVENT *const event = &level->sequence.events[i];
        if (event->type == GFS_ADD_ITEM
            || event->type == GFS_ADD_SECRET_REWARD) {
            const GF_ADD_ITEM_DATA *const data =
                (const GF_ADD_ITEM_DATA *)event->data;
            if (data->object_id < 0 || data->object_id >= O_NUMBER_OF) {
                continue;
            }
            if (data->inv_type == GF_INV_SECRET) {
                m_SecretInvItems[data->object_id] += data->quantity;
            } else if (data->inv_type == GF_INV_REGULAR) {
                m_Add2InvItems[data->object_id] += data->quantity;
            }
        } else if (event->type == GFS_REMOVE_WEAPONS) {
            m_RemoveWeapons = true;
        } else if (event->type == GFS_REMOVE_AMMO) {
            m_RemoveAmmo = true;
        } else if (event->type == GFS_REMOVE_FLARES) {
            m_RemoveFlares = true;
        } else if (event->type == GFS_REMOVE_MEDIPACKS) {
            m_RemoveMedipacks = true;
        }
    }
}

void GF_InventoryModifier_Apply(
    const GF_LEVEL *const level, const GF_INV_TYPE type)
{
    START_INFO *const start = Savegame_GetCurrentInfo(level);

    if (!start->has_pistols && m_Add2InvItems[O_PISTOL_ITEM]) {
        start->has_pistols = 1;
        Inv_AddItem(O_PISTOL_ITEM);
    }

    if (m_RemoveWeapons) {
        start->has_pistols = 0;
        start->has_magnums = 0;
        start->has_uzis = 0;
        start->has_shotgun = 0;
        start->has_m16 = 0;
        start->has_grenade = 0;
        start->has_harpoon = 0;
        start->gun_type = LGT_UNARMED;
        start->gun_status = LGS_ARMLESS;
    }

    if (m_RemoveAmmo) {
        start->m16_ammo = 0;
        start->grenade_ammo = 0;
        start->harpoon_ammo = 0;
        start->shotgun_ammo = 0;
        start->uzi_ammo = 0;
        start->magnum_ammo = 0;
        start->pistol_ammo = 0;
    }

    if (m_RemoveFlares) {
        start->flares = 0;
        m_RemoveFlares = false;
    }

    if (m_RemoveMedipacks) {
        start->large_medipacks = 0;
        start->small_medipacks = 0;
        m_RemoveMedipacks = false;
    }

    M_ModifyInventory_GunOrAmmo(start, type, LGT_MAGNUMS);
    M_ModifyInventory_GunOrAmmo(start, type, LGT_UZIS);
    M_ModifyInventory_GunOrAmmo(start, type, LGT_SHOTGUN);
    M_ModifyInventory_GunOrAmmo(start, type, LGT_HARPOON);
    M_ModifyInventory_GunOrAmmo(start, type, LGT_M16);
    M_ModifyInventory_GunOrAmmo(start, type, LGT_GRENADE);

    M_ModifyInventory_Item(type, O_FLARE_ITEM);
    M_ModifyInventory_Item(type, O_SMALL_MEDIPACK_ITEM);
    M_ModifyInventory_Item(type, O_LARGE_MEDIPACK_ITEM);
    M_ModifyInventory_Item(type, O_PICKUP_ITEM_1);
    M_ModifyInventory_Item(type, O_PICKUP_ITEM_2);
    M_ModifyInventory_Item(type, O_PUZZLE_ITEM_1);
    M_ModifyInventory_Item(type, O_PUZZLE_ITEM_2);
    M_ModifyInventory_Item(type, O_PUZZLE_ITEM_3);
    M_ModifyInventory_Item(type, O_PUZZLE_ITEM_4);
    M_ModifyInventory_Item(type, O_KEY_ITEM_1);
    M_ModifyInventory_Item(type, O_KEY_ITEM_2);
    M_ModifyInventory_Item(type, O_KEY_ITEM_3);
    M_ModifyInventory_Item(type, O_KEY_ITEM_4);
}
