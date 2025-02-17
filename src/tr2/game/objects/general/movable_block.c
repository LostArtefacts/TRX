#include "game/objects/general/movable_block.h"

#include "game/collide.h"
#include "game/input.h"
#include "game/item_actions.h"
#include "game/items.h"
#include "game/lara/control.h"
#include "game/objects/common.h"
#include "game/room.h"
#include "game/sound.h"
#include "global/utils.h"
#include "global/vars.h"

#include <libtrx/game/math.h>

#define LF_PPREADY 19

typedef enum {
    MOVABLE_BLOCK_STATE_STILL = 1,
    MOVABLE_BLOCK_STATE_PUSH = 2,
    MOVABLE_BLOCK_STATE_PULL = 3,
} MOVABLE_BLOCK_STATE;

static int16_t m_MovableBlockBounds[12] = {
    -300,
    +300,
    +0,
    +0,
    -WALL_L / 2 - LARA_RADIUS - 80,
    -WALL_L / 2,
    -10 * DEG_1,
    +10 * DEG_1,
    -30 * DEG_1,
    +30 * DEG_1,
    -10 * DEG_1,
    +10 * DEG_1,
};

int32_t MovableBlock_TestDestination(
    const ITEM *const item, const int32_t block_height)
{
    int16_t room_num = item->room_num;
    const SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);

    const int16_t floor = sector->floor.height;
    return floor == NO_HEIGHT || (floor == item->pos.y - block_height);
}

int32_t MovableBlock_TestPush(
    const ITEM *const item, const int32_t block_height, const uint16_t quadrant)
{
    if (!MovableBlock_TestDestination(item, block_height)) {
        return false;
    }

    int32_t x = item->pos.x;
    int32_t y = item->pos.y;
    int32_t z = item->pos.z;
    int16_t room_num = item->room_num;

    switch (quadrant) {
    case DIR_NORTH:
        z += WALL_L;
        break;
    case DIR_EAST:
        x += WALL_L;
        break;
    case DIR_SOUTH:
        z -= WALL_L;
        break;
    case DIR_WEST:
        x -= WALL_L;
        break;
    default:
        break;
    }

    COLL_INFO coll = {
        .quadrant = quadrant,
        .radius = 500,
        0,
    };
    if (Collide_CollideStaticObjects(&coll, x, y, z, room_num, 1000)) {
        return false;
    }

    const SECTOR *sector = Room_GetSector(x, y, z, &room_num);
    if (sector->floor.height != y) {
        return false;
    }

    Room_GetHeight(sector, x, y, z);
    if (g_HeightType != HT_WALL) {
        return false;
    }

    const int32_t y_max = y - block_height + 100;
    sector = Room_GetSector(x, y_max, z, &room_num);
    if (Room_GetCeiling(sector, x, y_max, z) > y_max) {
        return false;
    }

    return true;
}

int32_t MovableBlock_TestPull(
    const ITEM *const item, const int32_t block_height, const uint16_t quadrant)
{
    if (!MovableBlock_TestDestination(item, block_height)) {
        return false;
    }

    int32_t x_add = 0;
    int32_t z_add = 0;
    switch (quadrant) {
    case DIR_NORTH:
        z_add = -WALL_L;
        break;
    case DIR_EAST:
        x_add = -WALL_L;
        break;
    case DIR_SOUTH:
        z_add = WALL_L;
        break;
    case DIR_WEST:
        x_add = WALL_L;
        break;
    default:
        break;
    }

    int32_t x = item->pos.x + x_add;
    int32_t y = item->pos.y;
    int32_t z = item->pos.z + z_add;
    int16_t room_num = item->room_num;

    COLL_INFO coll = {
        .quadrant = quadrant,
        .radius = 500,
        0,
    };
    if (Collide_CollideStaticObjects(&coll, x, y, z, room_num, 1000)) {
        return false;
    }

    const SECTOR *sector = Room_GetSector(x, y, z, &room_num);
    if (sector->floor.height != y) {
        return false;
    }

    const int32_t y_min = y - block_height;
    sector = Room_GetSector(x, y_min, z, &room_num);
    if (sector->ceiling.height > y_min) {
        return false;
    }

    x += x_add;
    z += z_add;
    room_num = item->room_num;
    sector = Room_GetSector(x, y, z, &room_num);
    if (sector->floor.height != y) {
        return false;
    }

    sector = Room_GetSector(x, y - LARA_HEIGHT, z, &room_num);
    if (sector->ceiling.height > y - LARA_HEIGHT) {
        return false;
    }

    x = g_LaraItem->pos.x + x_add;
    z = g_LaraItem->pos.z + z_add;
    y = g_LaraItem->pos.y;
    room_num = g_LaraItem->room_num;
    Room_GetSector(x, y, z, &room_num);
    coll.quadrant = (quadrant + 2) & 3;
    coll.radius = LARA_RADIUS;
    if (Collide_CollideStaticObjects(&coll, x, y, z, room_num, LARA_HEIGHT)) {
        return false;
    }

    return true;
}

void MovableBlock_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (item->status != IS_INVISIBLE) {
        Room_AlterFloorHeight(item, -WALL_L);
    }
}

void MovableBlock_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

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
        ItemAction_Run(ITEM_ACTION_FLOOR_SHAKE, item);
        Sound_Effect(SFX_ENEMY_GRUNT, &item->pos, SPM_ALWAYS);
    }

    if (item->room_num != room_num) {
        Item_NewRoom(item_num, room_num);
    }

    if (item->status == IS_DEACTIVATED) {
        item->status = IS_INACTIVE;
        Item_RemoveActive(item_num);
        Room_AlterFloorHeight(item, -WALL_L);
        Room_TestTriggers(item);
    }
}

void MovableBlock_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);

    if (!g_Input.action || item->status == IS_ACTIVE || lara_item->gravity
        || lara_item->pos.y != item->pos.y) {
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

            if (!Item_TestPosition(m_MovableBlockBounds, item, lara_item)) {
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
        Item_TestAnimEqual(lara_item, LA_PUSHABLE_GRAB)
        && Item_TestFrameEqual(lara_item, LF_PPREADY)) {
        if (!Item_TestPosition(m_MovableBlockBounds, item, lara_item)) {
            return;
        }

        if (g_Input.forward) {
            if (!MovableBlock_TestPush(item, WALL_L, quadrant)) {
                return;
            }
            item->goal_anim_state = MOVABLE_BLOCK_STATE_PUSH;
            lara_item->goal_anim_state = LS_PUSH_BLOCK;
        } else if (g_Input.back) {
            if (!MovableBlock_TestPull(item, WALL_L, quadrant)) {
                return;
            }
            item->goal_anim_state = MOVABLE_BLOCK_STATE_PULL;
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

void MovableBlock_Draw(const ITEM *const item)
{
    if (item->status == IS_ACTIVE) {
        Object_DrawUnclippedItem(item);
    } else {
        Object_DrawAnimatingItem(item);
    }
}

void MovableBlock_Setup(OBJECT *const obj)
{
    obj->initialise_func = MovableBlock_Initialise;
    obj->control_func = MovableBlock_Control;
    obj->collision_func = MovableBlock_Collision;
    obj->draw_func = MovableBlock_Draw;
    obj->save_position = 1;
    obj->save_flags = 1;
    obj->save_anim = 1;
}
