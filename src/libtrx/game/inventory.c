#include "game/inventory.h"

#include "config.h"
#include "game/gun.h"
#include "game/inventory_ring/vars.h"
#include "game/lara.h"

INVENTORY_MODE g_Inv_Mode = INV_TITLE_MODE;

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
    INV_RING_SOURCE *const source =
        &g_InvRing_Source[inv_item->inv_pos < 100 ? RT_MAIN : RT_KEYS];

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
            if (source->items[i]->object_id == inv_obj_id) {
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
    // leave only the first item, which is the compass / stopwatch
    g_InvRing_Source[RT_MAIN].count = 1;
    g_InvRing_Source[RT_KEYS].count = 0;
    Inv_ClearSelection();
}
