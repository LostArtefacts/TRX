#include "game/game_flow.h"
#include "game/input.h"
#include "game/inventory.h"
#include "game/inventory_ring.h"
#include "game/items.h"
#include "game/lara/control.h"
#include "game/objects/common.h"
#include "game/objects/vars.h"
#include "game/sound.h"
#include "global/vars.h"

#define LF_USE_PUZZLE 80

static XYZ_32 m_PuzzleHolePosition = {
    .x = 0,
    .y = 0,
    .z = WALL_L / 2 - LARA_RADIUS - 85,
};

static int16_t m_PuzzleHoleBounds[12] = {
    // clang-format off
    -200,
    +200,
    +0,
    +0,
    +WALL_L / 2 - 200,
    +WALL_L / 2,
    -10 * DEG_1,
    +10 * DEG_1,
    -30 * DEG_1,
    +30 * DEG_1,
    -10 * DEG_1,
    +10 * DEG_1,
    // clang-format on
};

static void M_Refuse(const ITEM *lara_item);
static void M_Consume(
    ITEM *lara_item, ITEM *puzzle_hole_item, GAME_OBJECT_ID puzzle_obj_id);
static void M_MarkDone(ITEM *puzzle_hole_item);
static void M_SetupEmpty(OBJECT *obj);
static void M_SetupDone(OBJECT *obj);
static void M_HandleSave(ITEM *item, SAVEGAME_STAGE stage);
static void M_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);

static void M_Refuse(const ITEM *const lara_item)
{
    if (lara_item->pos.x != g_InteractPosition.x
        || lara_item->pos.y != g_InteractPosition.y
        || lara_item->pos.z != g_InteractPosition.z) {
        g_InteractPosition = lara_item->pos;
        Sound_Effect(SFX_LARA_NO, &lara_item->pos, SPM_ALWAYS);
    }
}

static void M_Consume(
    ITEM *const lara_item, ITEM *const puzzle_hole_item,
    const GAME_OBJECT_ID puzzle_obj_id)
{
    Inv_RemoveItem(puzzle_obj_id);
    Item_AlignPosition(&m_PuzzleHolePosition, puzzle_hole_item, lara_item);
    lara_item->goal_anim_state = LS_USE_PUZZLE;
    do {
        Lara_Animate(lara_item);
    } while (lara_item->current_anim_state != LS_USE_PUZZLE);
    lara_item->goal_anim_state = LS_STOP;
    g_Lara.gun_status = LGS_HANDS_BUSY;
    puzzle_hole_item->status = IS_ACTIVE;
    g_InteractPosition = lara_item->pos;
}

static void M_MarkDone(ITEM *const puzzle_hole_item)
{
    const GAME_OBJECT_ID done_obj_id = Object_GetCognate(
        puzzle_hole_item->object_id, g_ReceptacleToReceptacleDoneMap);
    if (done_obj_id != NO_OBJECT) {
        puzzle_hole_item->object_id = done_obj_id;
    }
}

static void M_SetupEmpty(OBJECT *const obj)
{
    obj->collision_func = M_Collision;
    obj->handle_save_func = M_HandleSave;
    obj->save_flags = 1;
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

    if (lara_item->current_anim_state != LS_STOP) {
        if (lara_item->current_anim_state != LS_USE_PUZZLE
            || !Item_TestPosition(m_PuzzleHoleBounds, item, lara_item)
            || !Item_TestFrameEqual(lara_item, LF_USE_PUZZLE)) {
            return;
        }

        M_MarkDone(item);
        return;
    }

    if ((g_Inv_Chosen == NO_OBJECT && !g_Input.action)
        || g_Lara.gun_status != LGS_ARMLESS || lara_item->gravity) {
        return;
    }

    if (!Item_TestPosition(m_PuzzleHoleBounds, item, lara_item)) {
        return;
    }

    if (item->status != IS_INACTIVE) {
        M_Refuse(lara_item);
        return;
    }

    if (g_Inv_Chosen == NO_OBJECT) {
        GF_ShowInventoryKeys(item->object_id);
        if (g_Inv_Chosen == NO_OBJECT && g_InvRing_Source[RT_KEYS].count > 0) {
            return;
        }
    }
    if (g_Inv_Chosen != NO_OBJECT) {
        g_InteractPosition.y = lara_item->pos.y - 1;
    }

    const GAME_OBJECT_ID puzzle_obj_id =
        Object_GetCognateInverse(item->object_id, g_KeyItemToReceptacleMap);
    const bool correct = g_Inv_Chosen == puzzle_obj_id;
    g_Inv_Chosen = NO_OBJECT;

    if (correct) {
        M_Consume(lara_item, item, puzzle_obj_id);
    } else {
        M_Refuse(lara_item);
    }
}

REGISTER_OBJECT(O_PUZZLE_HOLE_1, M_SetupEmpty)
REGISTER_OBJECT(O_PUZZLE_HOLE_2, M_SetupEmpty)
REGISTER_OBJECT(O_PUZZLE_HOLE_3, M_SetupEmpty)
REGISTER_OBJECT(O_PUZZLE_HOLE_4, M_SetupEmpty)
REGISTER_OBJECT(O_PUZZLE_DONE_1, M_SetupDone)
REGISTER_OBJECT(O_PUZZLE_DONE_2, M_SetupDone)
REGISTER_OBJECT(O_PUZZLE_DONE_3, M_SetupDone)
REGISTER_OBJECT(O_PUZZLE_DONE_4, M_SetupDone)
