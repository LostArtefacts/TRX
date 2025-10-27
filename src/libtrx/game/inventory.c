#include "game/inventory.h"

#include "config.h"
#include "game/game.h"
#include "game/gun.h"
#include "game/inventory_ring.h"
#include "game/inventory_ring/vars.h"
#include "game/lara.h"
#include "game/objects/vars.h"
#include "game/stats.h"

INVENTORY_MODE g_Inv_Mode = INV_TITLE_MODE;

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
    #if TR_VERSION >= 2
    case LGT_HARPOON: return &g_InvRing_Item_Harpoon;
    case LGT_M16:     return &g_InvRing_Item_M16;
    case LGT_GRENADE: return &g_InvRing_Item_Grenade;
    #endif
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
    #if TR_VERSION >= 2
    case LGT_HARPOON: return &g_InvRing_Item_HarpoonAmmo;
    case LGT_M16:     return &g_InvRing_Item_M16Ammo;
    case LGT_GRENADE: return &g_InvRing_Item_GrenadeAmmo;
    #endif
    default:          return nullptr;
    }
    // clang-format on
}

static void M_AddAmmo(const LARA_GUN_TYPE gun_type, const int32_t qty)
{
    AMMO_INFO *const ammo = Gun_GetAmmoInfo(gun_type);
    ammo->ammo += qty;
    CLAMPG(ammo->ammo, MAX_QTY);
}

static RING_TYPE M_GetRingType(const INVENTORY_ITEM *const inv_item)
{
    if (inv_item->inv_pos < 100) {
        return RT_MAIN;
    } else if (inv_item->inv_pos < 200) {
        return RT_KEYS;
    } else {
        return RT_OPTION;
    }
}

void Inv_AddGun(const LARA_GUN_TYPE gun_type)
{
    const OBJECT_ID gun_object = Gun_GetGunObject(gun_type);
    const OBJECT_ID ammo_object = Gun_GetAmmoObject(gun_type);
    LARA_INFO *const lara = Lara_GetLaraInfo();
    const int32_t ammo_qty = Gun_GetAmmoQuantity(gun_type);
    for (int32_t i = Inv_RequestItem(ammo_object); i > 0; i--) {
        Inv_RemoveItem(ammo_object);
        M_AddAmmo(gun_type, ammo_qty);
    }
    M_AddAmmo(gun_type, ammo_qty);
    Inv_InsertItem(M_GetGunInvItem(gun_type));
    if (lara->last_gun_type == LGT_UNARMED) {
        lara->last_gun_type = gun_type;
    }
    if (Gun_IsRifleType(gun_type)) {
#if TR_VERSION == 2
        if (lara->back_gun_obj_id == O_LARA) {
            lara->back_gun_obj_id = Gun_GetWeaponAnim(gun_type);
            lara->back_gun_type = gun_type;
        }
#endif
    }
    Item_GlobalReplace(gun_object, ammo_object);
}

void Inv_AddAmmo(const LARA_GUN_TYPE gun_type)
{
    const OBJECT_ID gun_object = Gun_GetGunObject(gun_type);
    const int32_t ammo_qty = Gun_GetAmmoQuantity(gun_type);
    if (Inv_RequestItem(gun_object)) {
        M_AddAmmo(gun_type, ammo_qty);
    } else {
        Inv_InsertItem(M_GetAmmoInvItem(gun_type));
    }
}

bool Inv_AddItemNTimes(const OBJECT_ID obj_id, const int32_t qty)
{
    bool result = false;
    for (int32_t i = 0; i < qty; i++) {
        result |= Inv_AddItem(obj_id);
    }
    return result;
}

OBJECT_ID Inv_GetItemOption(const OBJECT_ID obj_id)
{
    if (Object_IsType(obj_id, g_InvObjects)) {
        return obj_id;
    }
    return Object_GetCognate(obj_id, g_ItemToInvObjectMap);
}

void Inv_InsertItem(INVENTORY_ITEM *const inv_item)
{
    INV_RING_SOURCE *const source = &g_InvRing_Source[M_GetRingType(inv_item)];

    int32_t n;
    for (n = 0; n < source->count; n++) {
        if (source->items[n]->inv_pos > inv_item->inv_pos) {
            break;
        }
    }

    for (int32_t i = source->count; i > n - 1; i--) {
        source->items[i + 1] = source->items[i];
        source->qtys[i + 1] = source->qtys[i];
    }
    source->items[n] = inv_item;
    source->qtys[n] = 1;
    source->count++;
}

bool Inv_RemoveItem(const OBJECT_ID obj_id)
{
    const OBJECT_ID inv_obj_id = Inv_GetItemOption(obj_id);
    for (RING_TYPE ring_type = 0; ring_type < RT_NUMBER_OF; ring_type++) {
        INV_RING_SOURCE *const source = &g_InvRing_Source[ring_type];
        for (int32_t i = 0; i < source->count; i++) {
            if (source->items[i]->object_id != inv_obj_id) {
                continue;
            }

            source->qtys[i]--;

            if (g_Config.gameplay.fix_item_duplication_glitch) {
                for (int32_t j = i; j < source->count; j++) {
                    if (j == source->current) {
                        source->current = 0;
                    }
                }
            }

            if (source->qtys[i] == 0) {
                source->count--;
                for (int32_t j = i; j < source->count; j++) {
                    source->items[j] = source->items[j + 1];
                    source->qtys[j] = source->qtys[j + 1];
                }
            }
            return true;
        }
    }
    return false;
}

int32_t Inv_RequestItem(const OBJECT_ID obj_id)
{
    const OBJECT_ID inv_obj_id = Inv_GetItemOption(obj_id);
    for (RING_TYPE ring_type = 0; ring_type < RT_NUMBER_OF; ring_type++) {
        INV_RING_SOURCE *const source = &g_InvRing_Source[ring_type];
        for (int32_t i = 0; i < source->count; i++) {
            if (source->items[i] != nullptr
                && source->items[i]->object_id == inv_obj_id) {
                return source->qtys[i];
            }
        }
    }
    return 0;
}

void Inv_ClearSelection(void)
{
    g_InvRing_Source[RT_MAIN].current = 0;
    g_InvRing_Source[RT_KEYS].current = 0;
}

void Inv_RemoveAllItems(void)
{
    g_InvRing_Source[RT_MAIN].count = 0;
    g_InvRing_Source[RT_KEYS].count = 0;

    // Reset main ring
    Inv_AddItem(O_STOPWATCH_OPTION);
    Inv_AddItem(O_COMPASS_OPTION);

    Inv_ClearSelection();
}

bool Inv_AddItem(const OBJECT_ID obj_id)
{
    const OBJECT_ID inv_obj_id = Inv_GetItemOption(obj_id);
    const OBJECT *const object =
        Object_Get(inv_obj_id == NO_OBJECT ? obj_id : inv_obj_id);
    if (!object->loaded) {
        return false;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (Object_IsType(obj_id, g_GunObjects)) {
        Gun_UpdateLaraMeshes(obj_id);
        if (lara->gun_type == LGT_UNARMED) {
            lara->gun_type = Gun_GetType(obj_id);
            const bool hands_busy = lara->gun_status == LGS_HANDS_BUSY;
            lara->gun_status = LGS_ARMLESS;
            Gun_InitialiseNewWeapon();
            if (hands_busy) {
                lara->gun_status = LGS_HANDS_BUSY;
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
        Inv_InsertItem(&g_InvRing_Item_Compass);
        return true;

    case O_STOPWATCH_OPTION:
        Inv_InsertItem(&g_InvRing_Item_Stopwatch);
        return true;

    case O_PISTOL_ITEM:
    case O_PISTOL_OPTION:
        Inv_InsertItem(&g_InvRing_Item_Pistols);
        if (lara->last_gun_type == LGT_UNARMED) {
            lara->last_gun_type = LGT_PISTOLS;
        }
        return true;

    case O_SHOTGUN_ITEM:
    case O_SHOTGUN_OPTION:
        Inv_AddGun(LGT_SHOTGUN);
        return false;

    case O_MAGNUM_ITEM:
    case O_MAGNUM_OPTION:
        Inv_AddGun(LGT_MAGNUMS);
        return false;

    case O_UZI_ITEM:
    case O_UZI_OPTION:
        Inv_AddGun(LGT_UZIS);
        return false;

    case O_HARPOON_ITEM:
    case O_HARPOON_OPTION:
        Inv_AddGun(LGT_HARPOON);
        return false;

    case O_M16_ITEM:
    case O_M16_OPTION:
        Inv_AddGun(LGT_M16);
        return false;

    case O_GRENADE_ITEM:
    case O_GRENADE_OPTION:
        Inv_AddGun(LGT_GRENADE);
        return false;

    case O_SHOTGUN_AMMO_ITEM:
    case O_SHOTGUN_AMMO_OPTION:
        Inv_AddAmmo(LGT_SHOTGUN);
        return false;

    case O_MAGNUM_AMMO_ITEM:
    case O_MAGNUM_AMMO_OPTION:
        Inv_AddAmmo(LGT_MAGNUMS);
        return false;

    case O_UZI_AMMO_ITEM:
    case O_UZI_AMMO_OPTION:
        Inv_AddAmmo(LGT_UZIS);
        return false;

    case O_HARPOON_AMMO_ITEM:
    case O_HARPOON_AMMO_OPTION:
        Inv_AddAmmo(LGT_HARPOON);
        return false;

    case O_M16_AMMO_ITEM:
    case O_M16_AMMO_OPTION:
        Inv_AddAmmo(LGT_M16);
        return false;

    case O_GRENADE_AMMO_ITEM:
    case O_GRENADE_AMMO_OPTION:
        Inv_AddAmmo(LGT_GRENADE);
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

    case O_LEADBAR_ITEM:
    case O_LEADBAR_OPTION:
        Inv_InsertItem(&g_InvRing_Item_LeadBar);
        return true;

    case O_SCION_ITEM_1:
    case O_SCION_ITEM_2:
    case O_SCION_OPTION:
        Inv_InsertItem(&g_InvRing_Item_Scion);
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
