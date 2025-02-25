#include "game/inventory.h"

#include "game/inventory_ring.h"
#include "game/items.h"
#include "game/objects/vars.h"
#include "global/vars.h"

bool Inv_AddItem(const GAME_OBJECT_ID obj_id)
{
    const GAME_OBJECT_ID inv_obj_id = Inv_GetItemOption(obj_id);
    const OBJECT *const object =
        Object_Get(inv_obj_id == NO_OBJECT ? obj_id : inv_obj_id);
    if (!object->loaded) {
        return false;
    }

    for (RING_TYPE ring_type = 0; ring_type < RT_NUMBER_OF; ring_type++) {
        INV_RING_SOURCE *const source = &g_InvRing_Source[ring_type];
        for (int32_t i = 0; i < source->count; i++) {
            if (source->items[i]->object_id == inv_obj_id) {
                const int32_t qty =
                    obj_id == O_FLARES_ITEM ? FLARE_AMMO_QTY : 1;
                source->qtys[i] += qty;
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
        if (g_Lara.last_gun_type) {
            return true;
        }
        g_Lara.last_gun_type = 1;
        return true;

    case O_SHOTGUN_ITEM:
    case O_SHOTGUN_OPTION:
        for (int32_t i = Inv_RequestItem(O_SHOTGUN_AMMO_ITEM); i > 0; i--) {
            Inv_RemoveItem(O_SHOTGUN_AMMO_ITEM);
            g_Lara.shotgun_ammo.ammo += SHOTGUN_AMMO_QTY;
        }
        g_Lara.shotgun_ammo.ammo += SHOTGUN_AMMO_QTY;
        Inv_InsertItem(&g_InvRing_Item_Shotgun);
        if (g_Lara.last_gun_type == LGT_UNARMED) {
            g_Lara.last_gun_type = LGT_SHOTGUN;
        }
        if (!g_Lara.back_gun) {
            g_Lara.back_gun = O_LARA_SHOTGUN;
        }
        Item_GlobalReplace(O_SHOTGUN_ITEM, O_SHOTGUN_AMMO_ITEM);
        return false;

    case O_MAGNUM_ITEM:
    case O_MAGNUM_OPTION:
        for (int32_t i = Inv_RequestItem(O_MAGNUM_AMMO_ITEM); i > 0; i--) {
            Inv_RemoveItem(O_MAGNUM_AMMO_ITEM);
            g_Lara.magnum_ammo.ammo += MAGNUM_AMMO_QTY;
        }
        g_Lara.magnum_ammo.ammo += MAGNUM_AMMO_QTY;
        Inv_InsertItem(&g_InvRing_Item_Magnums);
        Item_GlobalReplace(O_MAGNUM_ITEM, O_MAGNUM_AMMO_ITEM);
        return false;

    case O_UZI_ITEM:
    case O_UZI_OPTION:
        for (int32_t i = Inv_RequestItem(O_UZI_AMMO_ITEM); i > 0; i--) {
            Inv_RemoveItem(O_UZI_AMMO_ITEM);
            g_Lara.uzi_ammo.ammo += UZI_AMMO_QTY;
        }
        g_Lara.uzi_ammo.ammo += UZI_AMMO_QTY;
        Inv_InsertItem(&g_InvRing_Item_Uzis);
        Item_GlobalReplace(O_UZI_ITEM, O_UZI_AMMO_ITEM);
        return false;

    case O_HARPOON_ITEM:
    case O_HARPOON_OPTION:
        for (int32_t i = Inv_RequestItem(O_HARPOON_AMMO_ITEM); i > 0; i--) {
            Inv_RemoveItem(O_HARPOON_AMMO_ITEM);
            g_Lara.harpoon_ammo.ammo += HARPOON_AMMO_QTY;
        }
        g_Lara.harpoon_ammo.ammo += HARPOON_AMMO_QTY;
        Inv_InsertItem(&g_InvRing_Item_Harpoon);
        Item_GlobalReplace(O_HARPOON_ITEM, O_HARPOON_AMMO_ITEM);
        return false;

    case O_M16_ITEM:
    case O_M16_OPTION:
        for (int32_t i = Inv_RequestItem(O_M16_AMMO_ITEM); i > 0; i--) {
            Inv_RemoveItem(O_M16_AMMO_ITEM);
            g_Lara.m16_ammo.ammo += M16_AMMO_QTY;
        }
        g_Lara.m16_ammo.ammo += M16_AMMO_QTY;
        Inv_InsertItem(&g_InvRing_Item_M16);
        Item_GlobalReplace(O_M16_ITEM, O_M16_AMMO_ITEM);
        return false;

    case O_GRENADE_ITEM:
    case O_GRENADE_OPTION:
        for (int32_t i = Inv_RequestItem(O_GRENADE_AMMO_ITEM); i > 0; i--) {
            Inv_RemoveItem(O_GRENADE_AMMO_ITEM);
            g_Lara.grenade_ammo.ammo += GRENADE_AMMO_QTY;
        }
        g_Lara.grenade_ammo.ammo += GRENADE_AMMO_QTY;
        Inv_InsertItem(&g_InvRing_Item_Grenade);
        Item_GlobalReplace(O_GRENADE_ITEM, O_GRENADE_AMMO_ITEM);
        return false;

    case O_SHOTGUN_AMMO_ITEM:
    case O_SHOTGUN_AMMO_OPTION:
        if (Inv_RequestItem(O_SHOTGUN_ITEM)) {
            g_Lara.shotgun_ammo.ammo += 12;
        } else {
            Inv_InsertItem(&g_InvRing_Item_ShotgunAmmo);
        }
        return false;

    case O_MAGNUM_AMMO_ITEM:
    case O_MAGNUM_AMMO_OPTION:
        if (Inv_RequestItem(O_MAGNUM_ITEM)) {
            g_Lara.magnum_ammo.ammo += 40;
        } else {
            Inv_InsertItem(&g_InvRing_Item_MagnumAmmo);
        }
        return false;

    case O_UZI_AMMO_ITEM:
    case O_UZI_AMMO_OPTION:
        if (Inv_RequestItem(O_UZI_ITEM)) {
            g_Lara.uzi_ammo.ammo += 80;
        } else {
            Inv_InsertItem(&g_InvRing_Item_UziAmmo);
        }
        return false;

    case O_HARPOON_AMMO_ITEM:
    case O_HARPOON_AMMO_OPTION:
        if (Inv_RequestItem(O_HARPOON_ITEM)) {
            g_Lara.harpoon_ammo.ammo += 3;
        } else {
            Inv_InsertItem(&g_InvRing_Item_HarpoonAmmo);
        }
        return false;

    case O_M16_AMMO_ITEM:
    case O_M16_AMMO_OPTION:
        if (Inv_RequestItem(O_M16_ITEM)) {
            g_Lara.m16_ammo.ammo += 40;
        } else {
            Inv_InsertItem(&g_InvRing_Item_M16Ammo);
        }
        return false;

    case O_GRENADE_AMMO_ITEM:
    case O_GRENADE_AMMO_OPTION:
        if (Inv_RequestItem(O_GRENADE_ITEM)) {
            g_Lara.grenade_ammo.ammo += 2;
        } else {
            Inv_InsertItem(&g_InvRing_Item_GrenadeAmmo);
        }
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
        for (int32_t i = 0; i < FLARE_AMMO_QTY; i++) {
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

    case O_SECRET_1:
        g_SaveGame.current_stats.secret_flags |= 1;
        return true;

    case O_SECRET_2:
        g_SaveGame.current_stats.secret_flags |= 2;
        return true;

    case O_SECRET_3:
        g_SaveGame.current_stats.secret_flags |= 4;
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
