#include "game/inventory.h"
#include "game/inventory/inventory_vars.h"
#include "game/items.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <stdbool.h>
#include <stdint.h>

bool Inv_AddItem(int32_t item_num)
{
    int32_t item_num_option = Inv_GetItemOption(item_num);
    if (!g_Objects[item_num_option].loaded) {
        return false;
    }

    for (int i = 0; i < g_InvMainObjects; i++) {
        INVENTORY_ITEM *inv_item = g_InvMainList[i];
        if (inv_item->object_number == item_num_option) {
            g_InvMainQtys[i]++;
            return true;
        }
    }

    for (int i = 0; i < g_InvKeysObjects; i++) {
        INVENTORY_ITEM *inv_item = g_InvKeysList[i];
        if (inv_item->object_number == item_num_option) {
            g_InvKeysQtys[i]++;
            return true;
        }
    }

    switch (item_num) {
    case O_GUN_ITEM:
    case O_GUN_OPTION:
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

    case O_PUZZLE_ITEM1:
    case O_PUZZLE_OPTION1:
        Inv_InsertItem(&g_InvItemPuzzle1);
        return true;

    case O_PUZZLE_ITEM2:
    case O_PUZZLE_OPTION2:
        Inv_InsertItem(&g_InvItemPuzzle2);
        return true;

    case O_PUZZLE_ITEM3:
    case O_PUZZLE_OPTION3:
        Inv_InsertItem(&g_InvItemPuzzle3);
        return true;

    case O_PUZZLE_ITEM4:
    case O_PUZZLE_OPTION4:
        Inv_InsertItem(&g_InvItemPuzzle4);
        return true;

    case O_LEADBAR_ITEM:
    case O_LEADBAR_OPTION:
        Inv_InsertItem(&g_InvItemLeadBar);
        return true;

    case O_KEY_ITEM1:
    case O_KEY_OPTION1:
        Inv_InsertItem(&g_InvItemKey1);
        return true;

    case O_KEY_ITEM2:
    case O_KEY_OPTION2:
        Inv_InsertItem(&g_InvItemKey2);
        return true;

    case O_KEY_ITEM3:
    case O_KEY_OPTION3:
        Inv_InsertItem(&g_InvItemKey3);
        return true;

    case O_KEY_ITEM4:
    case O_KEY_OPTION4:
        Inv_InsertItem(&g_InvItemKey4);
        return true;

    case O_PICKUP_ITEM1:
    case O_PICKUP_OPTION1:
        Inv_InsertItem(&g_InvItemPickup1);
        return true;

    case O_PICKUP_ITEM2:
    case O_PICKUP_OPTION2:
        Inv_InsertItem(&g_InvItemPickup2);
        return true;

    case O_SCION_ITEM:
    case O_SCION_ITEM2:
    case O_SCION_OPTION:
        Inv_InsertItem(&g_InvItemScion);
        return true;
    }

    return false;
}

void Inv_AddItemNTimes(int32_t item_num, int32_t qty)
{
    for (int i = 0; i < qty; i++) {
        Inv_AddItem(item_num);
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

int32_t Inv_RequestItem(int item_num)
{
    int32_t item_num_option = Inv_GetItemOption(item_num);

    for (int i = 0; i < g_InvMainObjects; i++) {
        if (g_InvMainList[i]->object_number == item_num_option) {
            return g_InvMainQtys[i];
        }
    }

    for (int i = 0; i < g_InvKeysObjects; i++) {
        if (g_InvKeysList[i]->object_number == item_num_option) {
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

bool Inv_RemoveItem(int32_t item_num)
{
    int32_t item_num_option = Inv_GetItemOption(item_num);

    for (int i = 0; i < g_InvMainObjects; i++) {
        if (g_InvMainList[i]->object_number == item_num_option) {
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
        if (g_InvKeysList[i]->object_number == item_num_option) {
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
        if (g_InvOptionList[i]->object_number == item_num_option) {
            g_InvOptionObjects--;
            for (int j = i; j < g_InvOptionObjects; j++) {
                g_InvOptionList[j] = g_InvOptionList[j + 1];
            }
            return true;
        }
    }

    return false;
}

int32_t Inv_GetItemOption(int32_t item_num)
{
    switch (item_num) {
    case O_GUN_ITEM:
    case O_GUN_OPTION:
        return O_GUN_OPTION;

    case O_SHOTGUN_ITEM:
    case O_SHOTGUN_OPTION:
        return O_SHOTGUN_OPTION;

    case O_MAGNUM_ITEM:
    case O_MAGNUM_OPTION:
        return O_MAGNUM_OPTION;

    case O_UZI_ITEM:
    case O_UZI_OPTION:
        return O_UZI_OPTION;

    case O_SG_AMMO_ITEM:
    case O_SG_AMMO_OPTION:
        return O_SG_AMMO_OPTION;

    case O_MAG_AMMO_ITEM:
    case O_MAG_AMMO_OPTION:
        return O_MAG_AMMO_OPTION;

    case O_UZI_AMMO_ITEM:
    case O_UZI_AMMO_OPTION:
        return O_UZI_AMMO_OPTION;

    case O_EXPLOSIVE_ITEM:
    case O_EXPLOSIVE_OPTION:
        return O_EXPLOSIVE_OPTION;

    case O_MEDI_ITEM:
    case O_MEDI_OPTION:
        return O_MEDI_OPTION;

    case O_BIGMEDI_ITEM:
    case O_BIGMEDI_OPTION:
        return O_BIGMEDI_OPTION;

    case O_PUZZLE_ITEM1:
    case O_PUZZLE_OPTION1:
        return O_PUZZLE_OPTION1;

    case O_PUZZLE_ITEM2:
    case O_PUZZLE_OPTION2:
        return O_PUZZLE_OPTION2;

    case O_PUZZLE_ITEM3:
    case O_PUZZLE_OPTION3:
        return O_PUZZLE_OPTION3;

    case O_PUZZLE_ITEM4:
    case O_PUZZLE_OPTION4:
        return O_PUZZLE_OPTION4;

    case O_LEADBAR_ITEM:
    case O_LEADBAR_OPTION:
        return O_LEADBAR_OPTION;

    case O_KEY_ITEM1:
    case O_KEY_OPTION1:
        return O_KEY_OPTION1;

    case O_KEY_ITEM2:
    case O_KEY_OPTION2:
        return O_KEY_OPTION2;

    case O_KEY_ITEM3:
    case O_KEY_OPTION3:
        return O_KEY_OPTION3;

    case O_KEY_ITEM4:
    case O_KEY_OPTION4:
        return O_KEY_OPTION4;

    case O_PICKUP_ITEM1:
    case O_PICKUP_OPTION1:
        return O_PICKUP_OPTION1;

    case O_PICKUP_ITEM2:
    case O_PICKUP_OPTION2:
        return O_PICKUP_OPTION2;

    case O_SCION_ITEM:
    case O_SCION_ITEM2:
    case O_SCION_OPTION:
        return O_SCION_OPTION;

    case O_DETAIL_OPTION:
    case O_SOUND_OPTION:
    case O_CONTROL_OPTION:
    case O_GAMMA_OPTION:
    case O_PASSPORT_OPTION:
    case O_MAP_OPTION:
    case O_PHOTO_OPTION:
        return item_num;
    }

    return -1;
}
