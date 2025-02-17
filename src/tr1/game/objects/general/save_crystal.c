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
static void M_Setup(OBJECT *obj);
static void M_Initialise(int16_t item_num);
static void M_Control(int16_t item_num);
static void M_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);

static const OBJECT_BOUNDS *M_Bounds(void)
{
    return &m_SaveCrystal_Bounds;
}

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    if (g_Config.gameplay.enable_save_crystals) {
        obj->control_func = M_Control;
        obj->collision_func = M_Collision;
        obj->save_flags = 1;
    }
    obj->bounds_func = M_Bounds;
    Object_SetReflective(O_SAVEGAME_ITEM, true);
}

static void M_Initialise(const int16_t item_num)
{
    if (g_Config.gameplay.enable_save_crystals) {
        Item_AddActive(item_num);
    } else {
        Item_Get(item_num)->status = IS_INVISIBLE;
    }
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    Item_Animate(item);
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
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

REGISTER_OBJECT(O_SAVEGAME_ITEM, M_Setup)
