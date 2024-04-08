#include "game/objects/general/puzzle_hole.h"

#include "game/input.h"
#include "game/inventory.h"
#include "game/inventory/inventory_vars.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/objects/general/keyhole.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/vars.h"

#define LF_USEPUZZLE 80

static XYZ_32 m_PuzzleHolePosition = { .x = 0,
                                       .y = 0,
                                       .z = WALL_L / 2 - LARA_RAD - 85 };

static const OBJECT_BOUNDS m_PuzzleHoleBounds = {
    .shift = {
        .min = { .x = -200, .y = 0, .z = WALL_L / 2 - 200, },
        .max = { .x = +200, .y = 0, .z = WALL_L / 2, },
    },
    .rot = {
        .min = {
            .x = -10 * PHD_DEGREE,
            .y = -30 * PHD_DEGREE,
            .z = -10 * PHD_DEGREE,
        },
        .max = {
            .x = +10 * PHD_DEGREE,
            .y = +30 * PHD_DEGREE,
            .z = +10 * PHD_DEGREE,
        },
    },
};

static const OBJECT_BOUNDS *PuzzleHole_Bounds(void);

static const OBJECT_BOUNDS *PuzzleHole_Bounds(void)
{
    return &m_PuzzleHoleBounds;
}

void PuzzleHole_Setup(OBJECT_INFO *obj)
{
    obj->collision = PuzzleHole_Collision;
    obj->save_flags = 1;
    obj->bounds = PuzzleHole_Bounds;
}

void PuzzleHole_SetupDone(OBJECT_INFO *obj)
{
    obj->save_flags = 1;
}

void PuzzleHole_Collision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &g_Items[item_num];
    const OBJECT_INFO *const obj = &g_Objects[item->object_number];

    if (lara_item->current_anim_state == LS_USE_PUZZLE) {
        if (!Lara_TestPosition(item, obj->bounds())) {
            return;
        }

        if (Item_TestFrameEqual(lara_item, LF_USEPUZZLE)) {
            switch (item->object_number) {
            case O_PUZZLE_HOLE1:
                item->object_number = O_PUZZLE_DONE1;
                break;

            case O_PUZZLE_HOLE2:
                item->object_number = O_PUZZLE_DONE2;
                break;

            case O_PUZZLE_HOLE3:
                item->object_number = O_PUZZLE_DONE3;
                break;

            case O_PUZZLE_HOLE4:
                item->object_number = O_PUZZLE_DONE4;
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
        || lara_item->gravity_status) {
        return;
    }

    if (!Lara_TestPosition(item, obj->bounds())) {
        return;
    }

    if (item->status != IS_NOT_ACTIVE) {
        Sound_Effect(SFX_LARA_NO, &lara_item->pos, SPM_NORMAL);
        return;
    }

    Inv_Display(INV_KEYS_MODE);
}
