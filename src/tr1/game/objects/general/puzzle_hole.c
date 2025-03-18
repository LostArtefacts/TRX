#include "game/game_flow.h"
#include "game/input.h"
#include "game/inventory.h"
#include "game/items.h"
#include "game/lara/common.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/vars.h"

#define LF_USEPUZZLE 80

static XYZ_32 m_PuzzleHolePosition = {
    .x = 0,
    .y = 0,
    .z = WALL_L / 2 - LARA_RAD - 85,
};

static const OBJECT_BOUNDS m_PuzzleHoleBounds = {
    .shift = {
        .min = { .x = -200, .y = 0, .z = WALL_L / 2 - 200, },
        .max = { .x = +200, .y = 0, .z = WALL_L / 2, },
    },
    .rot = {
        .min = {
            .x = -10 * DEG_1,
            .y = -30 * DEG_1,
            .z = -10 * DEG_1,
        },
        .max = {
            .x = +10 * DEG_1,
            .y = +30 * DEG_1,
            .z = +10 * DEG_1,
        },
    },
};

static const OBJECT_BOUNDS *M_Bounds(void);
static bool M_IsUsable(int16_t item_num);
static void M_Setup(OBJECT *obj);
static void M_SetupDone(OBJECT *obj);
static void M_HandleSave(ITEM *item, SAVEGAME_STAGE stage);
static void M_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);

static const OBJECT_BOUNDS *M_Bounds(void)
{
    return &m_PuzzleHoleBounds;
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
    obj->handle_save_func = M_HandleSave;
    obj->is_usable_func = M_IsUsable;
}

static void M_SetupDone(OBJECT *const obj)
{
    obj->save_flags = 1;
}

static void M_HandleSave(ITEM *const item, const SAVEGAME_STAGE stage)
{
    if (stage == SAVEGAME_STAGE_AFTER_LOAD) {
        if (item->status == IS_DEACTIVATED || item->status == IS_ACTIVE) {
            item->object_id += O_PUZZLE_DONE_1 - O_PUZZLE_HOLE_1;
        }
    }
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    const OBJECT *const obj = Object_Get(item->object_id);

    if (lara_item->current_anim_state == LS_USE_PUZZLE) {
        if (!Lara_TestPosition(item, obj->bounds_func())) {
            return;
        }

        if (Item_TestFrameEqual(lara_item, LF_USEPUZZLE)) {
            switch (item->object_id) {
            case O_PUZZLE_HOLE_1:
                item->object_id = O_PUZZLE_DONE_1;
                break;

            case O_PUZZLE_HOLE_2:
                item->object_id = O_PUZZLE_DONE_2;
                break;

            case O_PUZZLE_HOLE_3:
                item->object_id = O_PUZZLE_DONE_3;
                break;

            case O_PUZZLE_HOLE_4:
                item->object_id = O_PUZZLE_DONE_4;
                break;

            default:
                break;
            }
        }

        return;
    } else if (lara_item->current_anim_state != LS_STOP) {
        return;
    }

    if (g_Lara.interact_target.is_moving
        && g_Lara.interact_target.item_num == item_num) {
        Lara_AlignPosition(item, &m_PuzzleHolePosition);
        Lara_AnimateUntil(lara_item, LS_USE_PUZZLE);
        lara_item->goal_anim_state = LS_STOP;
        g_Lara.gun_status = LGS_HANDS_BUSY;
        item->status = IS_ACTIVE;
        g_Lara.interact_target.is_moving = false;
        g_Lara.interact_target.item_num = NO_OBJECT;
    }

    if (!g_Input.action || g_Lara.gun_status != LGS_ARMLESS
        || lara_item->gravity) {
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

REGISTER_OBJECT(O_PUZZLE_HOLE_1, M_Setup)
REGISTER_OBJECT(O_PUZZLE_HOLE_2, M_Setup)
REGISTER_OBJECT(O_PUZZLE_HOLE_3, M_Setup)
REGISTER_OBJECT(O_PUZZLE_HOLE_4, M_Setup)
REGISTER_OBJECT(O_PUZZLE_DONE_1, M_SetupDone)
REGISTER_OBJECT(O_PUZZLE_DONE_2, M_SetupDone)
REGISTER_OBJECT(O_PUZZLE_DONE_3, M_SetupDone)
REGISTER_OBJECT(O_PUZZLE_DONE_4, M_SetupDone)
