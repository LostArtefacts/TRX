#include "game/objects/general/keyhole.h"

#include "game/game_flow.h"
#include "game/input.h"
#include "game/items.h"
#include "game/lara/common.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/vars.h"

XYZ_32 g_KeyHolePosition = { 0, 0, WALL_L / 2 - LARA_RAD - 50 };

static const OBJECT_BOUNDS m_KeyHoleBounds = {
    .shift = {
        .min = { .x = -200, .y = +0, .z = +WALL_L / 2 - 200, },
        .max = { .x = +200, .y = +0, .z = +WALL_L / 2, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = -30 * DEG_1, .z = -10 * DEG_1, },
        .max = { .x = +10 * DEG_1, .y = +30 * DEG_1, .z = +10 * DEG_1, },
    },
};

static void M_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
static const OBJECT_BOUNDS *M_Bounds(void);
static bool M_IsUsable(int16_t item_num);
static void M_Setup(OBJECT *obj);

static void M_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll)
{
    ITEM *const item = Item_Get(item_num);
    const OBJECT *const obj = Object_Get(item->object_id);

    if (g_Lara.interact_target.is_moving
        && g_Lara.interact_target.item_num == item_num) {
        Lara_AlignPosition(item, &g_KeyHolePosition);
        Lara_AnimateUntil(lara_item, LS_USE_KEY);
        lara_item->goal_anim_state = LS_STOP;
        g_Lara.gun_status = LGS_HANDS_BUSY;
        item->status = IS_ACTIVE;
        g_Lara.interact_target.is_moving = false;
        g_Lara.interact_target.item_num = NO_OBJECT;
    }

    if (!g_Input.action || g_Lara.gun_status != LGS_ARMLESS
        || lara_item->gravity || lara_item->current_anim_state != LS_STOP) {
        return;
    }

    if (!Lara_TestPosition(item, obj->bounds_func())) {
        return;
    }

    if (item->status != IS_INACTIVE) {
        Sound_Effect(SFX_LARA_NO, &lara_item->pos, SPM_NORMAL);
        return;
    }

    GF_ShowInventoryKeys(item->object_id);
}

static const OBJECT_BOUNDS *M_Bounds(void)
{
    return &m_KeyHoleBounds;
}

static bool M_IsUsable(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    return item->status == IS_INACTIVE;
}

static void M_Setup(OBJECT *const obj)
{
    obj->collision_func = M_Collision;
    obj->save_flags = 1;
    obj->bounds_func = M_Bounds;
    obj->is_usable_func = M_IsUsable;
}

bool KeyHole_Trigger(int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (item->status == IS_ACTIVE && g_Lara.gun_status != LGS_HANDS_BUSY) {
        item->status = IS_DEACTIVATED;
        return true;
    }
    return false;
}

REGISTER_OBJECT(O_KEY_HOLE_1, M_Setup)
REGISTER_OBJECT(O_KEY_HOLE_2, M_Setup)
REGISTER_OBJECT(O_KEY_HOLE_3, M_Setup)
REGISTER_OBJECT(O_KEY_HOLE_4, M_Setup)
