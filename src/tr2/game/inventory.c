#include "game/inventory.h"

#include "game/game_flow.h"
#include "game/gun.h"
#include "game/inventory_ring.h"
#include "game/objects/vars.h"
#include "game/stats.h"
#include "global/vars.h"

#include <libtrx/game/game.h>
#include <libtrx/utils.h>

static int32_t M_GetFlareQuantity(void);
static INVENTORY_ITEM *M_GetGunInvItem(LARA_GUN_TYPE gun_type);
static INVENTORY_ITEM *M_GetAmmoInvItem(LARA_GUN_TYPE gun_type);
static void M_AddGun(LARA_GUN_TYPE gun_type);

static int32_t M_GetFlareQuantity(void)
{
    return Game_IsBonusFlagSet(GBF_JAPANESE) ? FLARE_AMMO_JAPANESE_QTY
                                             : FLARE_AMMO_QTY;
}

static INVENTORY_ITEM *M_GetGunInvItem(const LARA_GUN_TYPE gun_type)
{
    // clang-format off
    switch (gun_type) {
    case LGT_SHOTGUN: return &g_InvRing_Item_Shotgun;
    case LGT_MAGNUMS: return &g_InvRing_Item_Magnums;
    case LGT_UZIS:    return &g_InvRing_Item_Uzis;
    case LGT_HARPOON: return &g_InvRing_Item_Harpoon;
    case LGT_M16:     return &g_InvRing_Item_M16;
    case LGT_GRENADE: return &g_InvRing_Item_Grenade;
    default:          return nullptr;
    }
    // clang-format on
}

static INVENTORY_ITEM *M_GetAmmoInvItem(const LARA_GUN_TYPE gun_type)
{
    // clang-format off
    switch (gun_type) {
    case LGT_SHOTGUN: return &g_InvRing_Item_ShotgunAmmo;
    case LGT_MAGNUMS: return &g_InvRing_Item_MagnumAmmo;
    case LGT_UZIS:    return &g_InvRing_Item_UziAmmo;
    case LGT_HARPOON: return &g_InvRing_Item_HarpoonAmmo;
    case LGT_M16:     return &g_InvRing_Item_M16Ammo;
    case LGT_GRENADE: return &g_InvRing_Item_GrenadeAmmo;
    default:          return nullptr;
    }
    // clang-format on
}

static void M_AddGun(const LARA_GUN_TYPE gun_type)
{
    const GAME_OBJECT_ID gun_object = Gun_GetGunObject(gun_type);
    const GAME_OBJECT_ID ammo_object = Gun_GetAmmoObject(gun_type);
    AMMO_INFO *const ammo = Gun_GetAmmoInfo(gun_type);
    const int32_t ammo_qty = Gun_GetAmmoQuantity(gun_type);
    for (int32_t i = Inv_RequestItem(ammo_object); i > 0; i--) {
        Inv_RemoveItem(ammo_object);
        Inv_AddAmmo(ammo, ammo_qty);
    }
    Inv_AddAmmo(ammo, ammo_qty);
    Inv_InsertItem(M_GetGunInvItem(gun_type));
    if (g_Lara.last_gun_type == LGT_UNARMED) {
        g_Lara.last_gun_type = gun_type;
    }
    if (Gun_IsRifleType(gun_type)) {
        if (g_Lara.back_gun_obj_id == O_LARA) {
            g_Lara.back_gun_obj_id = Gun_GetWeaponAnim(gun_type);
            g_Lara.back_gun_type = gun_type;
        }
    }
    Item_GlobalReplace(gun_object, ammo_object);
}

static void M_AddAmmo(const LARA_GUN_TYPE gun_type)
{
    const GAME_OBJECT_ID gun_object = Gun_GetGunObject(gun_type);
    const int32_t ammo_qty = Gun_GetAmmoQuantity(gun_type);
    AMMO_INFO *const ammo = Gun_GetAmmoInfo(gun_type);
    if (Inv_RequestItem(gun_object)) {
        Inv_AddAmmo(ammo, ammo_qty);
    } else {
        Inv_InsertItem(M_GetAmmoInvItem(gun_type));
    }
}

bool Inv_AddItem(const GAME_OBJECT_ID obj_id)
{
    const GAME_OBJECT_ID inv_obj_id = Inv_GetItemOption(obj_id);
    const OBJECT *const object =
        Object_Get(inv_obj_id == NO_OBJECT ? obj_id : inv_obj_id);
    if (!object->loaded) {
        return false;
    }

    if (Object_IsType(obj_id, g_GunObjects)) {
        Gun_UpdateLaraMeshes(obj_id);
        if (g_Lara.gun_type == LGT_UNARMED) {
            g_Lara.gun_type = Gun_GetType(obj_id);
            const bool hands_busy = g_Lara.gun_status == LGS_HANDS_BUSY;
            g_Lara.gun_status = LGS_ARMLESS;
            Gun_InitialiseNewWeapon();
            if (hands_busy) {
                g_Lara.gun_status = LGS_HANDS_BUSY;
            }
        }
    }

    for (RING_TYPE ring_type = 0; ring_type < RT_NUMBER_OF; ring_type++) {
        INV_RING_SOURCE *const source = &g_InvRing_Source[ring_type];
        for (int32_t i = 0; i < source->count; i++) {
            if (source->items[i]->object_id == inv_obj_id) {
                const int32_t qty =
                    obj_id == O_FLARES_ITEM ? M_GetFlareQuantity() : 1;
                source->qtys[i] += qty;
                CLAMPG(source->qtys[i], MAX_QTY);
                return true;
            }
        }
    }

    switch (obj_id) {
    case O_COMPASS_OPTION:
    case O_COMPASS_ITEM:
        Inv_InsertItem(&g_InvRing_Item_Stopwatch);
        return true;

    case O_PISTOL_ITEM:
    case O_PISTOL_OPTION:
        Inv_InsertItem(&g_InvRing_Item_Pistols);
        if (g_Lara.last_gun_type == LGT_UNARMED) {
            g_Lara.last_gun_type = LGT_PISTOLS;
        }
        return true;

    case O_SHOTGUN_ITEM:
    case O_SHOTGUN_OPTION:
        M_AddGun(LGT_SHOTGUN);
        return false;

    case O_MAGNUM_ITEM:
    case O_MAGNUM_OPTION:
        M_AddGun(LGT_MAGNUMS);
        return false;

    case O_UZI_ITEM:
    case O_UZI_OPTION:
        M_AddGun(LGT_UZIS);
        return false;

    case O_HARPOON_ITEM:
    case O_HARPOON_OPTION:
        M_AddGun(LGT_HARPOON);
        return false;

    case O_M16_ITEM:
    case O_M16_OPTION:
        M_AddGun(LGT_M16);
        return false;

    case O_GRENADE_ITEM:
    case O_GRENADE_OPTION:
        M_AddGun(LGT_GRENADE);
        return false;

    case O_SHOTGUN_AMMO_ITEM:
    case O_SHOTGUN_AMMO_OPTION:
        M_AddAmmo(LGT_SHOTGUN);
        return false;

    case O_MAGNUM_AMMO_ITEM:
    case O_MAGNUM_AMMO_OPTION:
        M_AddAmmo(LGT_MAGNUMS);
        return false;

    case O_UZI_AMMO_ITEM:
    case O_UZI_AMMO_OPTION:
        M_AddAmmo(LGT_UZIS);
        return false;

    case O_HARPOON_AMMO_ITEM:
    case O_HARPOON_AMMO_OPTION:
        M_AddAmmo(LGT_HARPOON);
        return false;

    case O_M16_AMMO_ITEM:
    case O_M16_AMMO_OPTION:
        M_AddAmmo(LGT_M16);
        return false;

    case O_GRENADE_AMMO_ITEM:
    case O_GRENADE_AMMO_OPTION:
        M_AddAmmo(LGT_GRENADE);
        return false;

    case O_SMALL_MEDIPACK_ITEM:
    case O_SMALL_MEDIPACK_OPTION:
        Inv_InsertItem(&g_InvRing_Item_SmallMedi);
        return true;

    case O_LARGE_MEDIPACK_ITEM:
    case O_LARGE_MEDIPACK_OPTION:
        Inv_InsertItem(&g_InvRing_Item_LargeMedi);
        return true;

    case O_FLARES_ITEM:
    case O_FLARES_OPTION:
        for (int32_t i = 0; i < M_GetFlareQuantity(); i++) {
            Inv_AddItem(O_FLARE_ITEM);
        }
        return true;

    case O_FLARE_ITEM:
        Inv_InsertItem(&g_InvRing_Item_Flare);
        return true;

    case O_PUZZLE_ITEM_1:
    case O_PUZZLE_OPTION_1:
        Inv_InsertItem(&g_InvRing_Item_Puzzle1);
        return true;

    case O_PUZZLE_ITEM_2:
    case O_PUZZLE_OPTION_2:
        Inv_InsertItem(&g_InvRing_Item_Puzzle2);
        return true;

    case O_PUZZLE_ITEM_3:
    case O_PUZZLE_OPTION_3:
        Inv_InsertItem(&g_InvRing_Item_Puzzle3);
        return true;

    case O_PUZZLE_ITEM_4:
    case O_PUZZLE_OPTION_4:
        Inv_InsertItem(&g_InvRing_Item_Puzzle4);
        return true;

    case O_KEY_ITEM_1:
    case O_KEY_OPTION_1:
        Inv_InsertItem(&g_InvRing_Item_Key1);
        return true;

    case O_KEY_ITEM_2:
    case O_KEY_OPTION_2:
        Inv_InsertItem(&g_InvRing_Item_Key2);
        return true;

    case O_KEY_ITEM_3:
    case O_KEY_OPTION_3:
        Inv_InsertItem(&g_InvRing_Item_Key3);
        return true;

    case O_KEY_ITEM_4:
    case O_KEY_OPTION_4:
        Inv_InsertItem(&g_InvRing_Item_Key4);
        return true;

    case O_PICKUP_ITEM_1:
    case O_PICKUP_OPTION_1:
        Inv_InsertItem(&g_InvRing_Item_Pickup1);
        return true;

    case O_PICKUP_ITEM_2:
    case O_PICKUP_OPTION_2:
        Inv_InsertItem(&g_InvRing_Item_Pickup2);
        return true;

    default:
        return false;
    }
}

bool Inv_AddPickup(const ITEM *const item)
{
    switch (item->object_id) {
    case O_SECRET_1:
    case O_SECRET_2:
    case O_SECRET_3:
        Stats_MarkSecretCollected(item);
        if (Stats_CheckAllLevelSecretsCollected()) {
            GF_InventoryModifier_Apply(Game_GetCurrentLevel(), GF_INV_SECRET);
        }
        return true;
    default:
        break;
    }

    return Inv_AddItem(item->object_id);
}
