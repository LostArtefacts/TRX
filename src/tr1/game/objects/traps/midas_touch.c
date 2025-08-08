#include "game/game_flow.h"
#include "game/inventory.h"
#include "game/lara.h"
#include "game/objects/common.h"
#include "game/sound.h"
#include "global/vars.h"

#include <libtrx/game/camera.h>
#include <libtrx/game/input.h>
#include <libtrx/game/overlay.h>

#define M_RANGE_H (STEP_L * 2)
#define M_RANGE_V (STEP_L * 3)

typedef enum {
    M_ANIM_USE = 0,
    M_ANIM_DIE = 1,
} M_EXTRA_ANIM;

static const OBJECT_BOUNDS m_MidasTouch_Bounds = {
    .shift = {
        .min = { .x = -700, .y = +384 - 100, .z = -700, },
        .max = { .x = +700, .y = +384 + 100 + 512, .z = +700, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = -30 * DEG_1, .z = -10 * DEG_1, },
        .max = { .x = +10 * DEG_1, .y = +30 * DEG_1, .z = +10 * DEG_1, },
    },
};

static const OBJECT_BOUNDS *M_Bounds(void);
static bool M_IsUsable(int16_t item_num);
static void M_Setup(OBJECT *obj);
static void M_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);

static const OBJECT_BOUNDS *M_Bounds(void)
{
    return &m_MidasTouch_Bounds;
}

static bool M_IsUsable(const int16_t item_num)
{
    return g_LaraItem->current_anim_state != LS_USE_MIDAS;
}

static void M_Setup(OBJECT *const obj)
{
    obj->collision_func = M_Collision;
    obj->draw_func = Object_DrawDummyItem;
    obj->bounds_func = M_Bounds;
    obj->is_usable_func = M_IsUsable;
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    const OBJECT *const obj = Object_Get(item->object_id);

    const DIRECTION quadrant = (uint16_t)(lara_item->rot.y + DEG_45) / DEG_90;
    switch (quadrant) {
    case DIR_NORTH:
        item->rot.y = 0;
        break;
    case DIR_EAST:
        item->rot.y = DEG_90;
        break;
    case DIR_SOUTH:
        item->rot.y = -DEG_180;
        break;
    case DIR_WEST:
        item->rot.y = -DEG_90;
        break;
    default:
        break;
    }

    if (!lara_item->gravity && lara_item->current_anim_state == LS_STOP
        && lara_item->pos.x > item->pos.x - M_RANGE_H
        && lara_item->pos.x < item->pos.x + M_RANGE_H
        && lara_item->pos.y > item->pos.y - M_RANGE_V
        && lara_item->pos.y < item->pos.y + M_RANGE_V
        && lara_item->pos.z > item->pos.z - M_RANGE_H
        && lara_item->pos.z < item->pos.z + M_RANGE_H) {
        lara_item->current_anim_state = LS_DIE_MIDAS;
        lara_item->goal_anim_state = LS_DIE_MIDAS;
        Item_SwitchToObjAnim(lara_item, M_ANIM_DIE, 0, O_LARA_EXTRA);
        lara_item->hit_points = -1;
        lara_item->gravity = 0;
        g_Lara.extra_anim = true;
        g_Lara.air = -1;
        g_Lara.gun_status = LGS_HANDS_BUSY;
        g_Lara.gun_type = LGT_UNARMED;
        Camera_InvokeCinematic(lara_item, 0, 0);
        return;
    }

    if (g_Lara.interact_target.is_moving
        && g_Lara.interact_target.item_num == item_num) {
        lara_item->current_anim_state = LS_USE_MIDAS;
        lara_item->goal_anim_state = LS_USE_MIDAS;
        Item_SwitchToObjAnim(lara_item, M_ANIM_USE, 0, O_LARA_EXTRA);
        g_Lara.extra_anim = true;
        g_Lara.gun_status = LGS_HANDS_BUSY;
        g_Lara.interact_target.is_moving = false;
    }

    if (!g_Input.action || g_Lara.gun_status != LGS_ARMLESS
        || lara_item->gravity || lara_item->current_anim_state != LS_STOP) {
        return;
    }

    if (!Lara_TestPosition(item, obj->bounds_func())) {
        return;
    }

    if (!GF_ShowInventoryKeys(item->object_id)) {
        Lara_RefuseInteraction();
    }
}

REGISTER_OBJECT(O_MIDAS_TOUCH, M_Setup)
