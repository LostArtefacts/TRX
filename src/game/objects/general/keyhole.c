#include "game/objects/general/keyhole.h"

#include "game/input.h"
#include "game/inventory.h"
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
        .min = { .x = -10 * PHD_DEGREE, .y = -30 * PHD_DEGREE, .z = -10 * PHD_DEGREE, },
        .max = { .x = +10 * PHD_DEGREE, .y = +30 * PHD_DEGREE, .z = +10 * PHD_DEGREE, },
    },
};

static void M_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
;
static const OBJECT_BOUNDS *M_Bounds(void);

static void M_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll)
{
    ITEM *item = &g_Items[item_num];
    const OBJECT *const obj = &g_Objects[item->object_id];

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

    if (!Lara_TestPosition(item, obj->bounds())) {
        return;
    }

    if (item->status != IS_INACTIVE) {
        Sound_Effect(SFX_LARA_NO, &lara_item->pos, SPM_NORMAL);
        return;
    }

    Inv_Display(INV_KEYS_MODE);
}

static const OBJECT_BOUNDS *M_Bounds(void)
{
    return &m_KeyHoleBounds;
}

void KeyHole_Setup(OBJECT *obj)
{
    obj->collision = M_Collision;
    obj->save_flags = 1;
    obj->bounds = M_Bounds;
}

bool KeyHole_Trigger(int16_t item_num)
{
    ITEM *item = &g_Items[item_num];
    if (item->status == IS_ACTIVE && g_Lara.gun_status != LGS_HANDS_BUSY) {
        item->status = IS_DEACTIVATED;
        return true;
    }
    return false;
}
