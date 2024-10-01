#include "game/objects/traps/midas_touch.h"

#include "game/input.h"
#include "game/inventory.h"
#include "game/items.h"
#include "game/lara/common.h"
#include "game/objects/common.h"
#include "game/overlay.h"
#include "global/const.h"
#include "global/vars.h"

#define EXTRA_ANIM_PLACE_BAR 0
#define EXTRA_ANIM_DIE_GOLD 1
#define LF_PICKUP_GOLD_BAR 113
#define MIDAS_RANGE_H (STEP_L * 2)
#define MIDAS_RANGE_V (STEP_L * 3)

static const OBJECT_BOUNDS m_MidasTouch_Bounds = {
    .shift = {
        .min = { .x = -700, .y = +384 - 100, .z = -700, },
        .max = { .x = +700, .y = +384 + 100 + 512, .z = +700, },
    },
    .rot = {
        .min = { .x = -10 * PHD_DEGREE, .y = -30 * PHD_DEGREE, .z = -10 * PHD_DEGREE, },
        .max = { .x = +10 * PHD_DEGREE, .y = +30 * PHD_DEGREE, .z = +10 * PHD_DEGREE, },
    },
};

static const OBJECT_BOUNDS *M_Bounds(void)
{
    return &m_MidasTouch_Bounds;
}

void MidasTouch_Setup(OBJECT *obj)
{
    obj->collision = MidasTouch_Collision;
    obj->draw_routine = Object_DrawDummyItem;
    obj->bounds = M_Bounds;
}

void MidasTouch_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll)
{
    ITEM *item = &g_Items[item_num];
    const OBJECT *const obj = &g_Objects[item->object_id];

    DIRECTION quadrant = (uint16_t)(lara_item->rot.y + PHD_45) / PHD_90;
    switch (quadrant) {
    case DIR_NORTH:
        item->rot.y = 0;
        break;
    case DIR_EAST:
        item->rot.y = PHD_90;
        break;
    case DIR_SOUTH:
        item->rot.y = -PHD_180;
        break;
    case DIR_WEST:
        item->rot.y = -PHD_90;
        break;
    default:
        break;
    }

    if (lara_item->current_anim_state == LS_USE_MIDAS
        && g_Lara.interact_target.item_num == item_num
        && Item_TestFrameEqual(lara_item, LF_PICKUP_GOLD_BAR)) {
        Overlay_AddPickup(O_PUZZLE_ITEM_1);
        Inv_AddItem(O_PUZZLE_ITEM_1);
        g_Lara.interact_target.item_num = NO_OBJECT;
    }

    if (!lara_item->gravity && lara_item->current_anim_state == LS_STOP
        && lara_item->pos.x > item->pos.x - MIDAS_RANGE_H
        && lara_item->pos.x < item->pos.x + MIDAS_RANGE_H
        && lara_item->pos.y > item->pos.y - MIDAS_RANGE_V
        && lara_item->pos.y < item->pos.y + MIDAS_RANGE_V
        && lara_item->pos.z > item->pos.z - MIDAS_RANGE_H
        && lara_item->pos.z < item->pos.z + MIDAS_RANGE_H) {
        lara_item->current_anim_state = LS_DIE_MIDAS;
        lara_item->goal_anim_state = LS_DIE_MIDAS;
        Item_SwitchToObjAnim(lara_item, EXTRA_ANIM_DIE_GOLD, 0, O_LARA_EXTRA);
        lara_item->hit_points = -1;
        lara_item->gravity = 0;
        g_Lara.air = -1;
        g_Lara.gun_status = LGS_HANDS_BUSY;
        g_Lara.gun_type = LGT_UNARMED;
        g_Camera.type = CAM_CINEMATIC;
        g_CineFrame = 0;
        g_CinePosition.pos = lara_item->pos;
        g_CinePosition.rot.x = lara_item->rot.x;
        g_CinePosition.rot.y = lara_item->rot.y;
        g_CinePosition.rot.z = lara_item->rot.z;
        return;
    }

    if (g_Lara.interact_target.is_moving
        && g_Lara.interact_target.item_num == item_num) {
        lara_item->current_anim_state = LS_USE_MIDAS;
        lara_item->goal_anim_state = LS_USE_MIDAS;
        Item_SwitchToObjAnim(lara_item, EXTRA_ANIM_PLACE_BAR, 0, O_LARA_EXTRA);
        g_Lara.gun_status = LGS_HANDS_BUSY;
        g_Lara.interact_target.is_moving = false;
    }

    if (!g_Input.action || g_Lara.gun_status != LGS_ARMLESS
        || lara_item->gravity || lara_item->current_anim_state != LS_STOP) {
        return;
    }

    if (!Lara_TestPosition(item, obj->bounds())) {
        return;
    }

    Inv_Display(INV_KEYS_MODE);
}
