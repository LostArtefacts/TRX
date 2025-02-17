#include "game/objects/general/save_crystal.h"

#include "game/game_flow.h"
#include "game/input.h"
#include "game/items.h"
#include "game/lara/common.h"
#include "game/objects/common.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/config.h>

static const OBJECT_BOUNDS m_SaveCrystal_Bounds = {
    .shift = {
        .min = { .x = -256, .y = -100, .z = -256, },
        .max = { .x = +256, .y = +100, .z = +256, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = 0, .z = 0, },
        .max = { .x = +10 * DEG_1, .y = 0, .z = 0, },
    },
};

static const OBJECT_BOUNDS *M_Bounds(void);

static const OBJECT_BOUNDS *M_Bounds(void)
{
    return &m_SaveCrystal_Bounds;
}

void SaveCrystal_Setup(OBJECT *obj)
{
    obj->initialise_func = SaveCrystal_Initialise;
    if (g_Config.gameplay.enable_save_crystals) {
        obj->control_func = SaveCrystal_Control;
        obj->collision_func = SaveCrystal_Collision;
        obj->save_flags = 1;
    }
    obj->bounds_func = M_Bounds;
    Object_SetReflective(O_SAVEGAME_ITEM, true);
}

void SaveCrystal_Initialise(int16_t item_num)
{
    if (g_Config.gameplay.enable_save_crystals) {
        Item_AddActive(item_num);
    } else {
        Item_Get(item_num)->status = IS_INVISIBLE;
    }
}

void SaveCrystal_Control(int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    Item_Animate(item);
}

void SaveCrystal_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll)
{
    ITEM *const item = Item_Get(item_num);
    const OBJECT *const obj = Object_Get(item->object_id);

    Object_Collision(item_num, lara_item, coll);

    if (!g_Input.action || g_Lara.gun_status != LGS_ARMLESS
        || lara_item->gravity) {
        return;
    }

    if (lara_item->current_anim_state != LS_STOP) {
        return;
    }

    item->rot.y = lara_item->rot.y;
    item->rot.z = 0;
    item->rot.x = 0;
    if (!Lara_TestPosition(item, obj->bounds_func())) {
        return;
    }

    item->data = (void *)(intptr_t)(g_SaveCounter | 0x10000);
    GF_ShowInventory(INV_SAVE_CRYSTAL_MODE);
}
