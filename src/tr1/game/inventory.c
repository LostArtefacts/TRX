#include "game/inventory.h"

#include "game/gun.h"
#include "game/inventory_ring/vars.h"
#include "game/objects/common.h"
#include "game/objects/vars.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <libtrx/utils.h>

#include <stdint.h>

bool Inv_AddItem(const GAME_OBJECT_ID obj_id)
{
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

    const GAME_OBJECT_ID inv_obj_id = Inv_GetItemOption(obj_id);
    if (!Object_Get(inv_obj_id)->loaded) {
        return false;
    }

    for (RING_TYPE ring_type = 0; ring_type < RT_NUMBER_OF; ring_type++) {
        INV_RING_SOURCE *const source = &g_InvRing_Source[ring_type];
        for (int32_t i = 0; i < source->count; i++) {
            if (source->items[i]->object_id == inv_obj_id) {
                source->qtys[i]++;
                CLAMPG(source->qtys[i], MAX_QTY);
                return true;
            }
        }
    }

    switch (obj_id) {
    case O_PISTOL_ITEM:
    case O_PISTOL_OPTION:
        Inv_InsertItem(&g_InvRing_Item_Pistols);
        return true;

    case O_SHOTGUN_ITEM:
    case O_SHOTGUN_OPTION:
        for (int32_t i = Inv_RequestItem(O_SHOTGUN_AMMO_ITEM); i > 0; i--) {
            Inv_RemoveItem(O_SHOTGUN_AMMO_ITEM);
            Inv_AddAmmo(&g_Lara.shotgun_ammo, SHOTGUN_AMMO_QTY);
        }
        Inv_AddAmmo(&g_Lara.shotgun_ammo, SHOTGUN_AMMO_QTY);
        Inv_InsertItem(&g_InvRing_Item_Shotgun);
        Item_GlobalReplace(O_SHOTGUN_ITEM, O_SHOTGUN_AMMO_ITEM);
        return false;

    case O_MAGNUM_ITEM:
    case O_MAGNUM_OPTION:
        for (int32_t i = Inv_RequestItem(O_MAGNUM_AMMO_ITEM); i > 0; i--) {
            Inv_RemoveItem(O_MAGNUM_AMMO_ITEM);
            Inv_AddAmmo(&g_Lara.magnum_ammo, MAGNUM_AMMO_QTY);
        }
        Inv_AddAmmo(&g_Lara.magnum_ammo, MAGNUM_AMMO_QTY);
        Inv_InsertItem(&g_InvRing_Item_Magnum);
        Item_GlobalReplace(O_MAGNUM_ITEM, O_MAGNUM_AMMO_ITEM);
        return false;

    case O_UZI_ITEM:
    case O_UZI_OPTION:
        for (int32_t i = Inv_RequestItem(O_UZI_AMMO_ITEM); i > 0; i--) {
            Inv_RemoveItem(O_UZI_AMMO_ITEM);
            Inv_AddAmmo(&g_Lara.uzi_ammo, UZI_AMMO_QTY);
        }
        Inv_AddAmmo(&g_Lara.uzi_ammo, UZI_AMMO_QTY);
        Inv_InsertItem(&g_InvRing_Item_Uzi);
        Item_GlobalReplace(O_UZI_ITEM, O_UZI_AMMO_ITEM);
        return false;

    case O_SHOTGUN_AMMO_ITEM:
    case O_SHOTGUN_AMMO_OPTION:
        if (Inv_RequestItem(O_SHOTGUN_ITEM)) {
            Inv_AddAmmo(&g_Lara.shotgun_ammo, SHOTGUN_AMMO_QTY);
        } else {
            Inv_InsertItem(&g_InvRing_Item_ShotgunAmmo);
        }
        return false;

    case O_MAGNUM_AMMO_ITEM:
    case O_MAGNUM_AMMO_OPTION:
        if (Inv_RequestItem(O_MAGNUM_ITEM)) {
            Inv_AddAmmo(&g_Lara.magnum_ammo, MAGNUM_AMMO_QTY);
        } else {
            Inv_InsertItem(&g_InvRing_Item_MagnumAmmo);
        }
        return false;

    case O_UZI_AMMO_ITEM:
    case O_UZI_AMMO_OPTION:
        if (Inv_RequestItem(O_UZI_ITEM)) {
            Inv_AddAmmo(&g_Lara.uzi_ammo, UZI_AMMO_QTY);
        } else {
            Inv_InsertItem(&g_InvRing_Item_UziAmmo);
        }
        return false;

    case O_SMALL_MEDIPACK_ITEM:
    case O_SMALL_MEDIPACK_OPTION:
        Inv_InsertItem(&g_InvRing_Item_Medi);
        return true;

    case O_LARGE_MEDIPACK_ITEM:
    case O_LARGE_MEDIPACK_OPTION:
        Inv_InsertItem(&g_InvRing_Item_BigMedi);
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

    case O_LEADBAR_ITEM:
    case O_LEADBAR_OPTION:
        Inv_InsertItem(&g_InvRing_Item_LeadBar);
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

    case O_SCION_ITEM_1:
    case O_SCION_ITEM_2:
    case O_SCION_OPTION:
        Inv_InsertItem(&g_InvRing_Item_Scion);
        return true;

    default:
        return false;
    }

    return false;
}
