#include "game/objects/general/movable_block.h"

#include "game/input.h"
#include "game/items.h"
#include "game/lara/control.h"
#include "game/math.h"
#include "game/room.h"
#include "game/sound.h"
#include "global/funcs.h"
#include "global/utils.h"
#include "global/vars.h"

#define LF_PPREADY 19

typedef enum {
    MBS_STILL = 1,
    MBS_PUSH = 2,
    MBS_PULL = 3,
} MOVABLE_BLOCK_STATE;

int32_t __cdecl MovableBlock_TestDestination(
    const ITEM *const item, const int32_t block_height)
{
    int16_t room_num = item->room_num;
    const SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);

    const int16_t floor = sector->floor << 8;
    return floor == NO_HEIGHT || (floor == item->pos.y - block_height);
}

void __cdecl MovableBlock_Initialise(const int16_t item_num)
{
    ITEM *item = &g_Items[item_num];
    if (item->status != IS_INVISIBLE) {
        Room_AlterFloorHeight(&g_Items[item_num], -WALL_L);
    }
}

void __cdecl MovableBlock_Control(const int16_t item_num)
{
    ITEM *const item = &g_Items[item_num];

    if (item->flags & IF_ONE_SHOT) {
        Room_AlterFloorHeight(item, WALL_L);
        Item_Kill(item_num);
        return;
    }

    Item_Animate(item);

    int16_t room_num = item->room_num;
    const SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    const int32_t height =
        Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);

    if (item->pos.y < height) {
        item->gravity = 1;
    } else if (item->gravity) {
        item->gravity = 0;
        item->pos.y = height;
        item->status = IS_DEACTIVATED;
        floor_shake_effect(item);
        Sound_Effect(SFX_ENEMY_GRUNT, &item->pos, SPM_ALWAYS);
    }

    if (item->room_num != room_num) {
        Item_NewRoom(item_num, room_num);
    }

    if (item->status == IS_DEACTIVATED) {
        item->status = IS_INACTIVE;
        Item_RemoveActive(item_num);
        Room_AlterFloorHeight(item, -WALL_L);

        int16_t room_num = item->room_num;
        const SECTOR *const sector =
            Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
        Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);
        Room_TestTriggers(g_TriggerIndex, true);
    }
}

void __cdecl MovableBlock_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);

    if (!(g_Input & IN_ACTION) || item->status == IS_ACTIVE
        || lara_item->gravity || lara_item->pos.y != item->pos.y) {
        return;
    }

    const DIRECTION quadrant = Math_GetDirection(lara_item->rot.y);
    if (lara_item->current_anim_state == LS_STOP) {
        if (g_Lara.gun_status == LGS_ARMLESS) {
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

            if (!Item_TestPosition(g_MovableBlockBounds, item, lara_item)) {
                return;
            }

            int16_t room_num = lara_item->room_num;
            Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
            if (room_num != item->room_num) {
                return;
            }

            switch (quadrant) {
            case DIR_NORTH:
                lara_item->pos.z = ROUND_TO_SECTOR(lara_item->pos.z);
                lara_item->pos.z += WALL_L - LARA_RADIUS;
                break;
            case DIR_EAST:
                lara_item->pos.x = ROUND_TO_SECTOR(lara_item->pos.x);
                lara_item->pos.x += WALL_L - LARA_RADIUS;
                break;
            case DIR_SOUTH:
                lara_item->pos.z = ROUND_TO_SECTOR(lara_item->pos.z);
                lara_item->pos.z += LARA_RADIUS;
                break;
            case DIR_WEST:
                lara_item->pos.x = ROUND_TO_SECTOR(lara_item->pos.x);
                lara_item->pos.x += LARA_RADIUS;
                break;
            default:
                break;
            }

            lara_item->rot.y = item->rot.y;
            lara_item->goal_anim_state = LS_PP_READY;

            Lara_Animate(lara_item);

            if (lara_item->current_anim_state == LS_PP_READY) {
                g_Lara.gun_status = LGS_HANDS_BUSY;
            }
        }
    } else if (
        lara_item->current_anim_state == LS_PP_READY
        && lara_item->frame_num
            == g_Anims[LA_PUSHABLE_GRAB].frame_base + LF_PPREADY) {
        if (!Item_TestPosition(g_MovableBlockBounds, item, lara_item)) {
            return;
        }

        if (g_Input & IN_FORWARD) {
            if (!MovableBlock_TestPush(item, WALL_L, quadrant)) {
                return;
            }
            item->goal_anim_state = MBS_PUSH;
            lara_item->goal_anim_state = LS_PUSH_BLOCK;
        } else if (g_Input & IN_BACK) {
            if (!MovableBlock_TestPull(item, WALL_L, quadrant)) {
                return;
            }
            item->goal_anim_state = MBS_PULL;
            lara_item->goal_anim_state = LS_PULL_BLOCK;
        } else {
            return;
        }

        Item_AddActive(item_num);
        Room_AlterFloorHeight(item, WALL_L);
        item->status = IS_ACTIVE;
        Item_Animate(item);
        Lara_Animate(lara_item);
    }
}
