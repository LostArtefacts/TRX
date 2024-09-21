#include "game/gun.h"
#include "game/inventory.h"
#include "game/inventory/inventory_vars.h"
#include "game/items.h"
#include "game/objects/common.h"
#include "game/objects/vars.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <stdbool.h>
#include <stdint.h>

bool Inv_AddItem(const GAME_OBJECT_ID object_id)
{
    if (Object_IsObjectType(object_id, g_GunObjects)) {
        Gun_UpdateLaraMeshes(object_id);
        if (g_Lara.gun_type == LGT_UNARMED) {
            g_Lara.gun_type = Gun_GetType(object_id);
            g_Lara.gun_status = LGS_ARMLESS;
            Gun_InitialiseNewWeapon();
        }
    }

    const GAME_OBJECT_ID inv_object_id = Inv_GetItemOption(object_id);
    if (!g_Objects[inv_object_id].loaded) {
        return false;
    }

    for (int i = 0; i < g_InvMainObjects; i++) {
        INVENTORY_ITEM *inv_item = g_InvMainList[i];
        if (inv_item->object_id == inv_object_id) {
            g_InvMainQtys[i]++;
            return true;
        }
    }

    for (int i = 0; i < g_InvKeysObjects; i++) {
        INVENTORY_ITEM *inv_item = g_InvKeysList[i];
        if (inv_item->object_id == inv_object_id) {
            g_InvKeysQtys[i]++;
            return true;
        }
    }

    switch (object_id) {
    case O_PISTOL_ITEM:
    case O_PISTOL_OPTION:
        Inv_InsertItem(&g_InvItemPistols);
        return true;

    case O_SHOTGUN_ITEM:
    case O_SHOTGUN_OPTION:
        for (int i = Inv_RequestItem(O_SG_AMMO_ITEM); i > 0; i--) {
            Inv_RemoveItem(O_SG_AMMO_ITEM);
            g_Lara.shotgun.ammo += SHOTGUN_AMMO_QTY;
        }
        g_Lara.shotgun.ammo += SHOTGUN_AMMO_QTY;
        Inv_InsertItem(&g_InvItemShotgun);
        Item_GlobalReplace(O_SHOTGUN_ITEM, O_SG_AMMO_ITEM);
        return false;

    case O_MAGNUM_ITEM:
    case O_MAGNUM_OPTION:
        for (int i = Inv_RequestItem(O_MAG_AMMO_ITEM); i > 0; i--) {
            Inv_RemoveItem(O_MAG_AMMO_ITEM);
            g_Lara.magnums.ammo += MAGNUM_AMMO_QTY;
        }
        g_Lara.magnums.ammo += MAGNUM_AMMO_QTY;
        Inv_InsertItem(&g_InvItemMagnum);
        Item_GlobalReplace(O_MAGNUM_ITEM, O_MAG_AMMO_ITEM);
        return false;

    case O_UZI_ITEM:
    case O_UZI_OPTION:
        for (int i = Inv_RequestItem(O_UZI_AMMO_ITEM); i > 0; i--) {
            Inv_RemoveItem(O_UZI_AMMO_ITEM);
            g_Lara.uzis.ammo += UZI_AMMO_QTY;
        }
        g_Lara.uzis.ammo += UZI_AMMO_QTY;
        Inv_InsertItem(&g_InvItemUzi);
        Item_GlobalReplace(O_UZI_ITEM, O_UZI_AMMO_ITEM);
        return false;

    case O_SG_AMMO_ITEM:
    case O_SG_AMMO_OPTION:
        if (Inv_RequestItem(O_SHOTGUN_ITEM)) {
            g_Lara.shotgun.ammo += SHOTGUN_AMMO_QTY;
        } else {
            Inv_InsertItem(&g_InvItemShotgunAmmo);
        }
        return false;

    case O_MAG_AMMO_ITEM:
    case O_MAG_AMMO_OPTION:
        if (Inv_RequestItem(O_MAGNUM_ITEM)) {
            g_Lara.magnums.ammo += MAGNUM_AMMO_QTY;
        } else {
            Inv_InsertItem(&g_InvItemMagnumAmmo);
        }
        return false;

    case O_UZI_AMMO_ITEM:
    case O_UZI_AMMO_OPTION:
        if (Inv_RequestItem(O_UZI_ITEM)) {
            g_Lara.uzis.ammo += UZI_AMMO_QTY;
        } else {
            Inv_InsertItem(&g_InvItemUziAmmo);
        }
        return false;

    case O_MEDI_ITEM:
    case O_MEDI_OPTION:
        Inv_InsertItem(&g_InvItemMedi);
        return true;

    case O_BIGMEDI_ITEM:
    case O_BIGMEDI_OPTION:
        Inv_InsertItem(&g_InvItemBigMedi);
        return true;

    case O_PUZZLE_ITEM_1:
    case O_PUZZLE_OPTION_1:
        Inv_InsertItem(&g_InvItemPuzzle1);
        return true;

    case O_PUZZLE_ITEM_2:
    case O_PUZZLE_OPTION_2:
        Inv_InsertItem(&g_InvItemPuzzle2);
        return true;

    case O_PUZZLE_ITEM_3:
    case O_PUZZLE_OPTION_3:
        Inv_InsertItem(&g_InvItemPuzzle3);
        return true;

    case O_PUZZLE_ITEM_4:
    case O_PUZZLE_OPTION_4:
        Inv_InsertItem(&g_InvItemPuzzle4);
        return true;

    case O_LEADBAR_ITEM:
    case O_LEADBAR_OPTION:
        Inv_InsertItem(&g_InvItemLeadBar);
        return true;

    case O_KEY_ITEM_1:
    case O_KEY_OPTION_1:
        Inv_InsertItem(&g_InvItemKey1);
        return true;

    case O_KEY_ITEM_2:
    case O_KEY_OPTION_2:
        Inv_InsertItem(&g_InvItemKey2);
        return true;

    case O_KEY_ITEM_3:
    case O_KEY_OPTION_3:
        Inv_InsertItem(&g_InvItemKey3);
        return true;

    case O_KEY_ITEM_4:
    case O_KEY_OPTION_4:
        Inv_InsertItem(&g_InvItemKey4);
        return true;

    case O_PICKUP_ITEM_1:
    case O_PICKUP_OPTION_1:
        Inv_InsertItem(&g_InvItemPickup1);
        return true;

    case O_PICKUP_ITEM_2:
    case O_PICKUP_OPTION_2:
        Inv_InsertItem(&g_InvItemPickup2);
        return true;

    case O_SCION_ITEM_1:
    case O_SCION_ITEM_2:
    case O_SCION_OPTION:
        Inv_InsertItem(&g_InvItemScion);
        return true;

    default:
        return false;
    }

    return false;
}

void Inv_AddItemNTimes(const GAME_OBJECT_ID object_id, int32_t qty)
{
    for (int i = 0; i < qty; i++) {
        Inv_AddItem(object_id);
    }
}

void Inv_InsertItem(INVENTORY_ITEM *inv_item)
{
    int n;

    if (inv_item->inv_pos < 100) {
        for (n = 0; n < g_InvMainObjects; n++) {
            if (g_InvMainList[n]->inv_pos > inv_item->inv_pos) {
                break;
            }
        }

        if (n == g_InvMainObjects) {
            g_InvMainList[g_InvMainObjects] = inv_item;
            g_InvMainQtys[g_InvMainObjects] = 1;
            g_InvMainObjects++;
        } else {
            for (int i = g_InvMainObjects; i > n - 1; i--) {
                g_InvMainList[i + 1] = g_InvMainList[i];
                g_InvMainQtys[i + 1] = g_InvMainQtys[i];
            }
            g_InvMainList[n] = inv_item;
            g_InvMainQtys[n] = 1;
            g_InvMainObjects++;
        }
    } else {
        for (n = 0; n < g_InvKeysObjects; n++) {
            if (g_InvKeysList[n]->inv_pos > inv_item->inv_pos) {
                break;
            }
        }

        if (n == g_InvKeysObjects) {
            g_InvKeysList[g_InvKeysObjects] = inv_item;
            g_InvKeysQtys[g_InvKeysObjects] = 1;
            g_InvKeysObjects++;
        } else {
            for (int i = g_InvKeysObjects; i > n - 1; i--) {
                g_InvKeysList[i + 1] = g_InvKeysList[i];
                g_InvKeysQtys[i + 1] = g_InvKeysQtys[i];
            }
            g_InvKeysList[n] = inv_item;
            g_InvKeysQtys[n] = 1;
            g_InvKeysObjects++;
        }
    }
}

int32_t Inv_RequestItem(const GAME_OBJECT_ID object_id)
{
    const GAME_OBJECT_ID inv_object_id = Inv_GetItemOption(object_id);

    for (int i = 0; i < g_InvMainObjects; i++) {
        if (g_InvMainList[i]->object_id == inv_object_id) {
            return g_InvMainQtys[i];
        }
    }

    for (int i = 0; i < g_InvKeysObjects; i++) {
        if (g_InvKeysList[i]->object_id == inv_object_id) {
            return g_InvKeysQtys[i];
        }
    }

    return 0;
}

void Inv_RemoveAllItems(void)
{
    g_InvMainObjects = 1;
    g_InvMainCurrent = 0;

    g_InvKeysObjects = 0;
    g_InvKeysCurrent = 0;
}

bool Inv_RemoveItem(const GAME_OBJECT_ID object_id)
{
    const GAME_OBJECT_ID inv_object_id = Inv_GetItemOption(object_id);

    for (int i = 0; i < g_InvMainObjects; i++) {
        if (g_InvMainList[i]->object_id == inv_object_id) {
            g_InvMainQtys[i]--;
            if (g_InvMainQtys[i] > 0) {
                return true;
            }
            g_InvMainObjects--;
            for (int j = i; j < g_InvMainObjects; j++) {
                g_InvMainList[j] = g_InvMainList[j + 1];
                g_InvMainQtys[j] = g_InvMainQtys[j + 1];
            }
        }
    }

    for (int i = 0; i < g_InvKeysObjects; i++) {
        if (g_InvKeysList[i]->object_id == inv_object_id) {
            g_InvKeysQtys[i]--;
            if (g_InvKeysQtys[i] > 0) {
                return true;
            }
            g_InvKeysObjects--;
            for (int j = i; j < g_InvKeysObjects; j++) {
                g_InvKeysList[j] = g_InvKeysList[j + 1];
                g_InvKeysQtys[j] = g_InvKeysQtys[j + 1];
            }
            return true;
        }
    }

    for (int i = 0; i < g_InvOptionObjects; i++) {
        if (g_InvOptionList[i]->object_id == inv_object_id) {
            g_InvOptionObjects--;
            for (int j = i; j < g_InvOptionObjects; j++) {
                g_InvOptionList[j] = g_InvOptionList[j + 1];
            }
            return true;
        }
    }

    return false;
}

GAME_OBJECT_ID Inv_GetItemOption(const GAME_OBJECT_ID object_id)
{
    if (Object_IsObjectType(object_id, g_InvObjects)) {
        return object_id;
    }

    return Object_GetCognate(object_id, g_ItemToInvObjectMap);
}
