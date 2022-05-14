#include "game/objects/general/puzzle_hole.h"

#include "game/input.h"
#include "game/inventory/inventory_func.h"
#include "game/inventory/inventory_main.h"
#include "game/inventory/inventory_vars.h"
#include "game/lara/lara_main.h"
#include "game/objects/general/keyhole.h"
#include "game/sound.h"
#include "global/vars.h"

PHD_VECTOR g_PuzzleHolePosition = { 0, 0, WALL_L / 2 - LARA_RAD - 85 };

int16_t g_PuzzleHoleBounds[12] = {
    -200,
    +200,
    0,
    0,
    WALL_L / 2 - 200,
    WALL_L / 2,
    -10 * PHD_DEGREE,
    +10 * PHD_DEGREE,
    -30 * PHD_DEGREE,
    +30 * PHD_DEGREE,
    -10 * PHD_DEGREE,
    +10 * PHD_DEGREE,
};

void PuzzleHole_Setup(OBJECT_INFO *obj)
{
    obj->collision = PuzzleHole_Collision;
    obj->save_flags = 1;
}

void PuzzleHole_SetupDone(OBJECT_INFO *obj)
{
    obj->save_flags = 1;
}

void PuzzleHole_Collision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &g_Items[item_num];

    if (lara_item->current_anim_state == LS_USE_PUZZLE) {
        if (!Lara_TestPosition(item, g_PuzzleHoleBounds)) {
            return;
        }

        if (lara_item->frame_number == AF_USEPUZZLE) {
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

    if ((g_InvChosen == -1 && !g_Input.action)
        || g_Lara.gun_status != LGS_ARMLESS || lara_item->gravity_status) {
        return;
    }

    if (!Lara_TestPosition(item, g_PuzzleHoleBounds)) {
        return;
    }

    if (item->status != IS_NOT_ACTIVE) {
        if (lara_item->pos.x != g_PickUpX || lara_item->pos.y != g_PickUpY
            || lara_item->pos.z != g_PickUpZ) {
            g_PickUpX = lara_item->pos.x;
            g_PickUpY = lara_item->pos.y;
            g_PickUpZ = lara_item->pos.z;
            Sound_Effect(SFX_LARA_NO, &lara_item->pos, SPM_NORMAL);
        }
        return;
    }

    if (g_InvChosen == -1) {
        Inv_Display(INV_KEYS_MODE);
    } else {
        g_PickUpY = lara_item->pos.y - 1;
    }

    if (g_InvChosen == -1 && g_InvKeysObjects) {
        return;
    }

    if (g_InvChosen != -1) {
        g_PickUpY = lara_item->pos.y - 1;
    }

    int32_t correct = 0;
    switch (item->object_number) {
    case O_PUZZLE_HOLE1:
        if (g_InvChosen == O_PUZZLE_OPTION1) {
            Inv_RemoveItem(O_PUZZLE_OPTION1);
            correct = 1;
        }
        break;

    case O_PUZZLE_HOLE2:
        if (g_InvChosen == O_PUZZLE_OPTION2) {
            Inv_RemoveItem(O_PUZZLE_OPTION2);
            correct = 1;
        }
        break;

    case O_PUZZLE_HOLE3:
        if (g_InvChosen == O_PUZZLE_OPTION3) {
            Inv_RemoveItem(O_PUZZLE_OPTION3);
            correct = 1;
        }
        break;

    case O_PUZZLE_HOLE4:
        if (g_InvChosen == O_PUZZLE_OPTION4) {
            Inv_RemoveItem(O_PUZZLE_OPTION4);
            correct = 1;
        }
        break;

    default:
        break;
    }

    g_InvChosen = -1;
    if (correct) {
        Lara_AlignPosition(item, &g_PuzzleHolePosition);
        Lara_AnimateUntil(lara_item, LS_USE_PUZZLE);
        lara_item->goal_anim_state = LS_STOP;
        g_Lara.gun_status = LGS_HANDS_BUSY;
        item->status = IS_ACTIVE;
        g_PickUpX = lara_item->pos.x;
        g_PickUpY = lara_item->pos.y;
        g_PickUpZ = lara_item->pos.z;
    } else if (
        lara_item->pos.x != g_PickUpX || lara_item->pos.y != g_PickUpY
        || lara_item->pos.z != g_PickUpZ) {
        Sound_Effect(SFX_LARA_NO, &lara_item->pos, SPM_NORMAL);
        g_PickUpX = lara_item->pos.x;
        g_PickUpY = lara_item->pos.y;
        g_PickUpZ = lara_item->pos.z;
    }
}
