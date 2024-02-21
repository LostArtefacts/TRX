#include "game/objects/general/save_crystal.h"

#include "config.h"
#include "game/input.h"
#include "game/inventory.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/objects/common.h"
#include "global/const.h"
#include "global/vars.h"

static int16_t m_CrystalBounds[12] = {
    -256, +256, -100, +100, -256, +256, -10 * PHD_DEGREE, +10 * PHD_DEGREE,
    0,    0,    0,    0,
};

void SaveCrystal_Setup(OBJECT_INFO *obj)
{
    obj->initialise = SaveCrystal_Initialise;
    if (g_Config.enable_save_crystals) {
        obj->control = SaveCrystal_Control;
        obj->collision = SaveCrystal_Collision;
        obj->save_flags = 1;
    }
}

void SaveCrystal_Initialise(int16_t item_num)
{
    if (g_Config.enable_save_crystals) {
        Item_AddActive(item_num);
    } else {
        g_Items[item_num].status = IS_INVISIBLE;
    }
}

void SaveCrystal_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    if (item->data) {
        int32_t old_save_count = (int32_t)(intptr_t)item->data;
        if (g_SaveCounter > old_save_count) {
            item->status = IS_DEACTIVATED;
            Item_RemoveDrawn(item_num);
        }
    }
    Item_Animate(&g_Items[item_num]);
}

void SaveCrystal_Collision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &g_Items[item_num];

    Object_Collision(item_num, lara_item, coll);

    if (!g_Input.action || g_Lara.gun_status != LGS_ARMLESS
        || lara_item->gravity_status) {
        return;
    }

    if (lara_item->current_anim_state != LS_STOP) {
        return;
    }

    item->rot.y = lara_item->rot.y;
    item->rot.z = 0;
    item->rot.x = 0;
    if (!Lara_TestPosition(item, m_CrystalBounds)) {
        return;
    }

    g_Items[item_num].data = ((void *)(intptr_t)g_SaveCounter);
    Inv_Display(INV_SAVE_CRYSTAL_MODE);
}
