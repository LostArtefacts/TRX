#include "game/objects/general/puzzle_hole.h"

#include "game/input.h"
#include "game/inventory/backpack.h"
#include "game/inventory/common.h"
#include "game/items.h"
#include "game/lara/control.h"
#include "game/objects/common.h"
#include "game/objects/vars.h"
#include "game/sound.h"
#include "global/vars.h"

#define LF_USE_PUZZLE 80

static void M_Refuse(const ITEM *lara_item);
static void M_Consume(
    ITEM *lara_item, ITEM *puzzle_hole_item, GAME_OBJECT_ID puzzle_object_id);
static void M_MarkDone(ITEM *puzzle_hole_item);

static void M_Refuse(const ITEM *const lara_item)
{
    if (lara_item->pos.x != g_PickupPosition.x
        || lara_item->pos.y != g_PickupPosition.y
        || lara_item->pos.z != g_PickupPosition.z) {
        g_PickupPosition = lara_item->pos;
        Sound_Effect(SFX_LARA_NO, &lara_item->pos, SPM_ALWAYS);
    }
}

static void M_Consume(
    ITEM *const lara_item, ITEM *const puzzle_hole_item,
    const GAME_OBJECT_ID puzzle_object_id)
{
    Inv_RemoveItem(puzzle_object_id);
    Item_AlignPosition(&g_PuzzleHolePosition, puzzle_hole_item, lara_item);
    lara_item->goal_anim_state = LS_USE_PUZZLE;
    do {
        Lara_Animate(lara_item);
    } while (lara_item->current_anim_state != LS_USE_PUZZLE);
    lara_item->goal_anim_state = LS_STOP;
    g_Lara.gun_status = LGS_HANDS_BUSY;
    puzzle_hole_item->status = IS_ACTIVE;
    g_PickupPosition = lara_item->pos;
}

static void M_MarkDone(ITEM *const puzzle_hole_item)
{
    const GAME_OBJECT_ID done_object_id = Object_GetCognate(
        puzzle_hole_item->object_id, g_ReceptacleToReceptacleDoneMap);
    if (done_object_id != NO_OBJECT) {
        puzzle_hole_item->object_id = done_object_id;
    }
}

void __cdecl PuzzleHole_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);

    if (lara_item->current_anim_state != LS_STOP) {
        if (lara_item->current_anim_state != LS_USE_PUZZLE
            || !Item_TestPosition(g_PuzzleHoleBounds, item, lara_item)
            || lara_item->frame_num
                != g_Anims[LA_USE_PUZZLE].frame_base + LF_USE_PUZZLE) {
            return;
        }

        M_MarkDone(item);
        return;
    }

    if ((g_Inv_Chosen == NO_OBJECT && !(g_Input & IN_ACTION))
        || g_Lara.gun_status != LGS_ARMLESS || lara_item->gravity) {
        return;
    }

    if (!Item_TestPosition(g_PuzzleHoleBounds, item, lara_item)) {
        return;
    }

    if (item->status != IS_INACTIVE) {
        M_Refuse(lara_item);
        return;
    }

    if (g_Inv_Chosen == NO_OBJECT) {
        Inv_Display(INV_KEYS_MODE);
        if (g_Inv_Chosen == NO_OBJECT && g_Inv_KeyObjectsCount > 0) {
            return;
        }
    }
    if (g_Inv_Chosen != NO_OBJECT) {
        g_PickupPosition.y = lara_item->pos.y - 1;
    }

    const GAME_OBJECT_ID puzzle_object_id =
        Object_GetCognateInverse(item->object_id, g_KeyItemToReceptacleMap);
    const bool correct = g_Inv_Chosen == puzzle_object_id;
    g_Inv_Chosen = NO_OBJECT;

    if (correct) {
        M_Consume(lara_item, item, puzzle_object_id);
    } else {
        M_Refuse(lara_item);
    }
}
