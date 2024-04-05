#include "game/objects/general/save_crystal.h"

#include "config.h"
#include "game/input.h"
#include "game/inventory.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/objects/common.h"
#include "global/const.h"
#include "global/vars.h"

static const OBJECT_BOUNDS m_CrystalBounds = {
    .shift = {
        .min = { .x = -256, .y = -100, .z = -256, },
        .max = { .x = +256, .y = +100, .z = +256, },
    },
    .rot = {
        .min = { .x = -10 * PHD_DEGREE, .y = 0, .z = 0, },
        .max = { .x = +10 * PHD_DEGREE, .y = 0, .z = 0, },
    },
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
    Item_Animate(item);
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
    if (!Lara_TestPosition(item, &m_CrystalBounds)) {
        return;
    }

    item->data = (void *)(intptr_t)(g_SaveCounter | 0x10000);
    Inv_Display(INV_SAVE_CRYSTAL_MODE);
}
