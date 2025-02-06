#include "game/objects/traps/movable_block.h"

#include "game/camera.h"
#include "game/collide.h"
#include "game/game_flow.h"
#include "game/input.h"
#include "game/item_actions.h"
#include "game/items.h"
#include "game/lara/common.h"
#include "game/objects/common.h"
#include "game/objects/vars.h"
#include "game/random.h"
#include "game/room.h"
#include "game/sound.h"
#include "game/spawn.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/utils.h>

#define LF_PPREADY 19

typedef enum {
    MOVABLE_BLOCK_STATE_STILL = 1,
    MOVABLE_BLOCK_STATE_PUSH = 2,
    MOVABLE_BLOCK_STATE_PULL = 3,
} MOVABLE_BLOCK_STATE;

static const OBJECT_BOUNDS m_MovableBlock_Bounds = {
    .shift = {
        .min = { .x = -300, .y = 0, .z = -WALL_L / 2 - (LARA_RAD + 80), },
        .max = { .x = +300, .y = 0, .z = -WALL_L / 2, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = -30 * DEG_1, .z = -10 * DEG_1, },
        .max = { .x = +10 * DEG_1, .y = +30 * DEG_1, .z = +10 * DEG_1, },
    },
};

static const OBJECT_BOUNDS *M_Bounds(void);
static bool M_TestDoor(ITEM *lara_item, COLL_INFO *coll);
static bool M_TestDestination(ITEM *item, int32_t block_height);
static bool M_TestPush(ITEM *item, int32_t block_height, DIRECTION quadrant);
static bool M_TestPull(ITEM *item, int32_t block_height, DIRECTION quadrant);
static bool M_TestDeathCollision(ITEM *item, const ITEM *lara);
static void M_KillLara(const ITEM *item, ITEM *lara);

static const OBJECT_BOUNDS *M_Bounds(void)
{
    return &m_MovableBlock_Bounds;
}

static bool M_TestDoor(ITEM *lara_item, COLL_INFO *coll)
{
    // OG fix: stop pushing blocks through doors

    const int32_t shift = 8; // constant shift to avoid overflow errors
    const int32_t max_dist = SQUARE((WALL_L * 2) >> shift);
    for (int item_num = 0; item_num < Item_GetLevelCount(); item_num++) {
        ITEM *const item = Item_Get(item_num);
        if (!Object_IsType(item->object_id, g_DoorObjects)) {
            continue;
        }

        const int32_t dx = (item->pos.x - lara_item->pos.x) >> shift;
        const int32_t dy = (item->pos.y - lara_item->pos.y) >> shift;
        const int32_t dz = (item->pos.z - lara_item->pos.z) >> shift;
        const int32_t dist = SQUARE(dx) + SQUARE(dy) + SQUARE(dz);
        if (dist > max_dist) {
            continue;
        }

        if (Lara_TestBoundsCollide(item, coll->radius)
            && Collide_TestCollision(item, lara_item)) {
            return true;
        }
    }
    return false;
}

static bool M_TestDestination(ITEM *item, int32_t block_height)
{
    int16_t room_num = item->room_num;
    const SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    if (Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z)
        == NO_HEIGHT) {
        return true;
    }

    if (Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z)
        != item->pos.y - block_height) {
        return false;
    }

    return true;
}

static bool M_TestPush(ITEM *item, int32_t block_height, DIRECTION quadrant)
{
    if (!M_TestDestination(item, block_height)) {
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

    const SECTOR *sector = Room_GetSector(x, y, z, &room_num);
    COLL_INFO coll;
    coll.quadrant = quadrant;
    coll.radius = 500;
    if (Collide_CollideStaticObjects(&coll, x, y, z, room_num, 1000)) {
        return false;
    }

    if (Room_GetHeight(sector, x, y, z) != y) {
        return false;
    }

    sector = Room_GetSector(x, y - block_height, z, &room_num);
    if (Room_GetCeiling(sector, x, y - block_height, z) > y - block_height) {
        return false;
    }

    return true;
}

static bool M_TestPull(ITEM *item, int32_t block_height, DIRECTION quadrant)
{
    if (!M_TestDestination(item, block_height)) {
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
    const SECTOR *sector = Room_GetSector(x, y, z, &room_num);
    COLL_INFO coll;
    coll.quadrant = quadrant;
    coll.radius = 500;
    if (Collide_CollideStaticObjects(&coll, x, y, z, room_num, 1000)) {
        return false;
    }

    if (Room_GetHeight(sector, x, y, z) != y) {
        return false;
    }

    sector = Room_GetSector(x, y - block_height, z, &room_num);
    if (Room_GetCeiling(sector, x, y - block_height, z) > y - block_height) {
        return false;
    }

    x += x_add;
    z += z_add;
    room_num = item->room_num;
    sector = Room_GetSector(x, y, z, &room_num);

    if (Room_GetHeight(sector, x, y, z) != y) {
        return false;
    }

    sector = Room_GetSector(x, y - LARA_HEIGHT, z, &room_num);
    if (Room_GetCeiling(sector, x, y - LARA_HEIGHT, z) > y - LARA_HEIGHT) {
        return false;
    }

    x = g_LaraItem->pos.x + x_add;
    y = g_LaraItem->pos.y;
    z = g_LaraItem->pos.z + z_add;
    room_num = g_LaraItem->room_num;
    sector = Room_GetSector(x, y, z, &room_num);
    coll.radius = LARA_RAD;
    coll.quadrant = (quadrant + 2) & 3;
    if (Collide_CollideStaticObjects(&coll, x, y, z, room_num, LARA_HEIGHT)) {
        return false;
    }

    return true;
}

static bool M_TestDeathCollision(ITEM *const item, const ITEM *const lara)
{
    return g_GameFlow.enable_killer_pushblocks && item->gravity
        && Lara_TestBoundsCollide(item, 0);
}

static void M_KillLara(const ITEM *const item, ITEM *const lara)
{
    if (lara->hit_points <= 0) {
        return;
    }

    lara->hit_points = -1;
    lara->pos.y = lara->floor;
    lara->speed = 0;
    lara->fall_speed = 0;
    lara->gravity = false;
    lara->rot.x = 0;
    lara->rot.z = 0;
    lara->enable_shadow = false;
    lara->current_anim_state = LS_SPECIAL;
    lara->goal_anim_state = LS_SPECIAL;
    Item_SwitchToAnim(lara, LA_ROLLING_BALL_DEATH, 0);

    for (int32_t i = 0; i < 15; i++) {
        const int32_t x = lara->pos.x + (Random_GetControl() - 0x4000) / 256;
        const int32_t z = lara->pos.z + (Random_GetControl() - 0x4000) / 256;
        const int32_t y = lara->pos.y - Random_GetControl() / 64;
        const int32_t d = lara->rot.y + (Random_GetControl() - 0x4000) / 8;
        Spawn_Blood(x, y, z, item->speed * 2, d, lara->room_num);
    }

    if (!Object_Get(O_CAMERA_TARGET)->loaded) {
        return;
    }

    const int16_t target_num = Item_Spawn(lara, O_CAMERA_TARGET);
    if (target_num != NO_ITEM) {
        ITEM *const target = Item_Get(target_num);
        target->rot.y = g_Camera.target_angle;
        target->pos.y = lara->floor - WALL_L;
        Lara_SetDeathCameraTarget(target_num);
    }
}

void MovableBlock_Setup(OBJECT *obj)
{
    obj->initialise = MovableBlock_Initialise;
    obj->control = MovableBlock_Control;
    obj->draw_routine = MovableBlock_Draw;
    obj->collision = MovableBlock_Collision;
    obj->save_position = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
    obj->bounds = M_Bounds;
}

void MovableBlock_Initialise(int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (item->status != IS_INVISIBLE && item->pos.y >= Item_GetHeight(item)) {
        Room_AlterFloorHeight(item, -WALL_L);
    }
}

void MovableBlock_Control(int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (item->flags & IF_ONE_SHOT) {
        Room_AlterFloorHeight(item, WALL_L);
        Item_Kill(item_num);
        return;
    }

    Item_Animate(item);

    int16_t room_num = item->room_num;
    const SECTOR *sector = Room_GetSector(
        item->pos.x, item->pos.y - STEP_L / 2, item->pos.z, &room_num);
    const int32_t height = Room_GetHeight(
        sector, item->pos.x, item->pos.y - STEP_L / 2, item->pos.z);

    if (item->pos.y < height) {
        item->gravity = 1;
    } else if (item->gravity) {
        item->gravity = 0;
        item->pos.y = height;
        item->status = IS_DEACTIVATED;
        ItemAction_Run(ITEM_ACTION_FLOOR_SHAKE, item);
        Sound_Effect(SFX_T_REX_FOOTSTOMP, &item->pos, SPM_NORMAL);
    } else if (
        item->pos.y >= height && !item->gravity
        && !(bool)(intptr_t)item->priv) {
        item->status = IS_INACTIVE;
        Item_RemoveActive(item_num);
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

void MovableBlock_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll)
{
    ITEM *const item = Item_Get(item_num);
    const OBJECT *const obj = Object_Get(item->object_id);

    if (M_TestDeathCollision(item, lara_item)) {
        M_KillLara(item, lara_item);
        return;
    }

    if (item->current_anim_state == MOVABLE_BLOCK_STATE_STILL) {
        item->priv = (void *)false;
    }

    if (!g_Input.action || item->status == IS_ACTIVE || lara_item->gravity
        || lara_item->pos.y != item->pos.y) {
        return;
    }

    DIRECTION quadrant = ((uint16_t)lara_item->rot.y + DEG_45) / DEG_90;
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

        if (!Lara_TestPosition(item, obj->bounds())) {
            return;
        }

        // OG fix: stop pushing blocks through doors
        if (M_TestDoor(lara_item, coll)) {
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
        default:
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

        if (!Lara_TestPosition(item, obj->bounds())) {
            return;
        }

        if (g_Input.forward) {
            if (!M_TestPush(item, WALL_L, quadrant)) {
                return;
            }
            item->goal_anim_state = MOVABLE_BLOCK_STATE_PUSH;
            lara_item->goal_anim_state = LS_PUSH_BLOCK;
        } else if (g_Input.back) {
            if (!M_TestPull(item, WALL_L, quadrant)) {
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
        item->priv = (void *)true;
    }
}

void MovableBlock_Draw(ITEM *item)
{
    if (item->status == IS_ACTIVE) {
        Object_DrawUnclippedItem(item);
    } else {
        Object_DrawAnimatingItem(item);
    }
}
