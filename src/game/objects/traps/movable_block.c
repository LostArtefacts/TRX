#include "game/objects/traps/movable_block.h"

#include "game/collide.h"
#include "game/effect_routines/dino_stomp.h"
#include "game/input.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/objects/common.h"
#include "game/room.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/vars.h"
#include "util.h"

#include <stdbool.h>

#define LF_PPREADY 19

typedef enum {
    MBS_STILL = 1,
    MBS_PUSH = 2,
    MBS_PULL = 3,
} MOVABLE_BLOCK_STATE;

static OBJECT_BOUNDS m_MovingBlockBounds = {
    .min_shift_x = -300,
    .max_shift_x = +300,
    .min_shift_y = 0,
    .max_shift_y = 0,
    .min_shift_z = -WALL_L / 2 - (LARA_RAD + 80),
    .max_shift_z = -WALL_L / 2,
    .min_rot_x = -10 * PHD_DEGREE,
    .max_rot_x = +10 * PHD_DEGREE,
    .min_rot_y = -30 * PHD_DEGREE,
    .max_rot_y = +30 * PHD_DEGREE,
    .min_rot_z = -10 * PHD_DEGREE,
    .max_rot_z = +10 * PHD_DEGREE,
};

static bool MovableBlock_TestDoor(ITEM_INFO *lara_item, COLL_INFO *coll);
static bool MovableBlock_TestDestination(ITEM_INFO *item, int32_t block_height);
static bool MovableBlock_TestPush(
    ITEM_INFO *item, int32_t block_height, DIRECTION quadrant);
static bool MovableBlock_TestPull(
    ITEM_INFO *item, int32_t block_height, DIRECTION quadrant);

static bool MovableBlock_TestDoor(ITEM_INFO *lara_item, COLL_INFO *coll)
{
    // OG fix: stop pushing blocks through doors
    int32_t max_dist = SQUARE((WALL_L * 2) >> 8);
    for (int item_num = 0; item_num < g_LevelItemCount; item_num++) {
        ITEM_INFO *item = &g_Items[item_num];
        int32_t dx = (item->pos.x - lara_item->pos.x) >> 8;
        int32_t dy = (item->pos.y - lara_item->pos.y) >> 8;
        int32_t dz = (item->pos.z - lara_item->pos.z) >> 8;
        int32_t dist = SQUARE(dx) + SQUARE(dy) + SQUARE(dz);

        if (dist > max_dist) {
            continue;
        }

        if ((item->object_number < O_DOOR_TYPE1
             || item->object_number > O_DOOR_TYPE8)) {
            continue;
        }

        if (Lara_TestBoundsCollide(item, coll->radius)
            && Collide_TestCollision(item, lara_item)) {
            return true;
        }
    }
    return false;
}

static bool MovableBlock_TestDestination(ITEM_INFO *item, int32_t block_height)
{
    int16_t room_num = item->room_number;
    FLOOR_INFO *floor =
        Room_GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
    if (Room_GetHeight(floor, item->pos.x, item->pos.y, item->pos.z)
        == NO_HEIGHT) {
        return true;
    }

    if (Room_GetHeight(floor, item->pos.x, item->pos.y, item->pos.z)
        != item->pos.y - block_height) {
        return false;
    }

    return true;
}

static bool MovableBlock_TestPush(
    ITEM_INFO *item, int32_t block_height, DIRECTION quadrant)
{
    if (!MovableBlock_TestDestination(item, block_height)) {
        return false;
    }

    int32_t x = item->pos.x;
    int32_t y = item->pos.y;
    int32_t z = item->pos.z;
    int16_t room_num = item->room_number;

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
    }

    FLOOR_INFO *floor = Room_GetFloor(x, y, z, &room_num);
    COLL_INFO coll;
    coll.quadrant = quadrant;
    coll.radius = 500;
    if (Collide_CollideStaticObjects(&coll, x, y, z, room_num, 1000)) {
        return false;
    }

    if (Room_GetHeight(floor, x, y, z) != y) {
        return false;
    }

    floor = Room_GetFloor(x, y - block_height, z, &room_num);
    if (Room_GetCeiling(floor, x, y - block_height, z) > y - block_height) {
        return false;
    }

    return true;
}

static bool MovableBlock_TestPull(
    ITEM_INFO *item, int32_t block_height, DIRECTION quadrant)
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
    }

    int32_t x = item->pos.x + x_add;
    int32_t y = item->pos.y;
    int32_t z = item->pos.z + z_add;

    int16_t room_num = item->room_number;
    FLOOR_INFO *floor = Room_GetFloor(x, y, z, &room_num);
    COLL_INFO coll;
    coll.quadrant = quadrant;
    coll.radius = 500;
    if (Collide_CollideStaticObjects(&coll, x, y, z, room_num, 1000)) {
        return false;
    }

    if (Room_GetHeight(floor, x, y, z) != y) {
        return false;
    }

    floor = Room_GetFloor(x, y - block_height, z, &room_num);
    if (Room_GetCeiling(floor, x, y - block_height, z) > y - block_height) {
        return false;
    }

    x += x_add;
    z += z_add;
    room_num = item->room_number;
    floor = Room_GetFloor(x, y, z, &room_num);

    if (Room_GetHeight(floor, x, y, z) != y) {
        return false;
    }

    floor = Room_GetFloor(x, y - LARA_HEIGHT, z, &room_num);
    if (Room_GetCeiling(floor, x, y - LARA_HEIGHT, z) > y - LARA_HEIGHT) {
        return false;
    }

    x = g_LaraItem->pos.x + x_add;
    y = g_LaraItem->pos.y;
    z = g_LaraItem->pos.z + z_add;
    room_num = g_LaraItem->room_number;
    floor = Room_GetFloor(x, y, z, &room_num);
    coll.radius = LARA_RAD;
    coll.quadrant = (quadrant + 2) & 3;
    if (Collide_CollideStaticObjects(&coll, x, y, z, room_num, LARA_HEIGHT)) {
        return false;
    }

    return true;
}

void MovableBlock_Setup(OBJECT_INFO *obj)
{
    obj->initialise = MovableBlock_Initialise;
    obj->control = MovableBlock_Control;
    obj->draw_routine = MovableBlock_Draw;
    obj->collision = MovableBlock_Collision;
    obj->save_position = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

void MovableBlock_Initialise(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];

    if (item->status != IS_INVISIBLE && item->pos.y >= Item_GetHeight(item)) {
        Room_AlterFloorHeight(item, -WALL_L);
    }
}

void MovableBlock_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];

    if (item->flags & IF_ONESHOT) {
        Room_AlterFloorHeight(item, WALL_L);
        Item_Kill(item_num);
        return;
    }

    Item_Animate(item);

    int16_t room_num = item->room_number;
    FLOOR_INFO *floor = Room_GetFloor(
        item->pos.x, item->pos.y - STEP_L / 2, item->pos.z, &room_num);
    int32_t height = Room_GetHeight(
        floor, item->pos.x, item->pos.y - STEP_L / 2, item->pos.z);

    if (item->pos.y < height) {
        item->gravity_status = 1;
    } else if (item->gravity_status) {
        item->gravity_status = 0;
        item->pos.y = height;
        item->status = IS_DEACTIVATED;
        FX_DinoStomp(item);
        Sound_Effect(SFX_T_REX_FOOTSTOMP, &item->pos, SPM_NORMAL);
    } else if (
        item->pos.y >= height && !item->gravity_status
        && !(bool)(intptr_t)item->priv) {
        item->status = IS_NOT_ACTIVE;
        Item_RemoveActive(item_num);
    }

    if (item->room_number != room_num) {
        Item_NewRoom(item_num, room_num);
    }

    if (item->status == IS_DEACTIVATED) {
        item->status = IS_NOT_ACTIVE;
        Item_RemoveActive(item_num);
        Room_AlterFloorHeight(item, -WALL_L);

        room_num = item->room_number;
        floor = Room_GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
        Room_GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);
        Room_TestTriggers(g_TriggerIndex, true);
    }
}

void MovableBlock_Collision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &g_Items[item_num];

    if (item->current_anim_state == MBS_STILL) {
        item->priv = (void *)false;
    }

    if (!g_Input.action || item->status == IS_ACTIVE
        || lara_item->gravity_status || lara_item->pos.y != item->pos.y) {
        return;
    }

    DIRECTION quadrant = ((uint16_t)lara_item->rot.y + PHD_45) / PHD_90;
    if (lara_item->current_anim_state == LS_STOP) {
        if (g_Input.forward || g_Input.back
            || g_Lara.gun_status != LGS_ARMLESS) {
            return;
        }

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
        }

        if (!Lara_TestPosition(item, &m_MovingBlockBounds)) {
            return;
        }

        // OG fix: stop pushing blocks through doors
        if (MovableBlock_TestDoor(lara_item, coll)) {
            return;
        }

        switch (quadrant) {
        case DIR_NORTH:
            lara_item->pos.z &= -WALL_L;
            lara_item->pos.z += WALL_L - LARA_RAD;
            break;
        case DIR_SOUTH:
            lara_item->pos.z &= -WALL_L;
            lara_item->pos.z += LARA_RAD;
            break;
        case DIR_EAST:
            lara_item->pos.x &= -WALL_L;
            lara_item->pos.x += WALL_L - LARA_RAD;
            break;
        case DIR_WEST:
            lara_item->pos.x &= -WALL_L;
            lara_item->pos.x += LARA_RAD;
            break;
        }

        lara_item->rot.y = item->rot.y;
        lara_item->goal_anim_state = LS_PP_READY;

        Lara_Animate(lara_item);

        if (lara_item->current_anim_state == LS_PP_READY) {
            g_Lara.gun_status = LGS_HANDS_BUSY;
        }
    } else if (Item_TestAnimEqual(lara_item, LA_PUSHABLE_GRAB)) {
        if (!Item_TestFrameEqual(lara_item, LF_PPREADY)) {
            return;
        }

        if (!Lara_TestPosition(item, &m_MovingBlockBounds)) {
            return;
        }

        if (g_Input.forward) {
            if (!MovableBlock_TestPush(item, WALL_L, quadrant)) {
                return;
            }
            item->goal_anim_state = MBS_PUSH;
            lara_item->goal_anim_state = LS_PUSH_BLOCK;
        } else if (g_Input.back) {
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
        item->priv = (void *)true;
    }
}

void MovableBlock_Draw(ITEM_INFO *item)
{
    if (item->status == IS_ACTIVE) {
        Object_DrawUnclippedItem(item);
    } else {
        Object_DrawAnimatingItem(item);
    }
}
