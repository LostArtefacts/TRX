#include "game/game_flow.h"
#include "game/item_actions.h"
#include "game/lara.h"
#include "game/objects/common.h"
#include "game/objects/vars.h"
#include "game/spawn.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/collision.h>
#include <libtrx/game/input.h>
#include <libtrx/game/items/walkable.h>
#include <libtrx/game/lara/const.h>
#include <libtrx/game/objects/traps/movable_block.h>
#include <libtrx/game/random.h>
#include <libtrx/game/sound.h>
#include <libtrx/utils.h>

#define LF_PPREADY 19

typedef enum {
    MOVABLE_BLOCK_STATE_STILL = 1,
    MOVABLE_BLOCK_STATE_PUSH = 2,
    MOVABLE_BLOCK_STATE_PULL = 3,
} MOVABLE_BLOCK_STATE;

static const OBJECT_BOUNDS m_MovableBlock_Bounds = {
    .shift = {
        .min = { .x = -300, .y = 0, .z = -WALL_L / 2 - (LARA_RADIUS + 80), },
        .max = { .x = +300, .y = 0, .z = -WALL_L / 2, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = -30 * DEG_1, .z = -10 * DEG_1, },
        .max = { .x = +10 * DEG_1, .y = +30 * DEG_1, .z = +10 * DEG_1, },
    },
};

static const OBJECT_BOUNDS *M_Bounds(void);
static bool M_IsAgainstFloor(const ITEM *item);
static bool M_IsAgainstCeiling(const ITEM *item);
static int16_t M_GetFloorHeight(
    const ITEM *item, int32_t x, int32_t y, int32_t z, int16_t height);
static int16_t M_GetCeilingHeight(
    const ITEM *item, int32_t x, int32_t y, int32_t z, int16_t height);
static bool M_IsItemOnTop(const ITEM *item, int32_t x, int32_t z);
static bool M_TestDoor(ITEM *lara_item, COLL_INFO *coll);
static bool M_TestCurrentSector(ITEM *item, int32_t block_height);
static bool M_TestPush(ITEM *item, int32_t block_height, DIRECTION quadrant);
static bool M_TestPull(ITEM *item, int32_t block_height, DIRECTION quadrant);
static bool M_TestDeathCollision(const ITEM *item, const ITEM *lara);
static bool M_TestEmbedCollision(const ITEM *item, const ITEM *lara);
static void M_KillLara(const ITEM *item, ITEM *lara);
static void M_Setup(OBJECT *obj);
static void M_HandleSave(ITEM *item, SAVEGAME_STAGE stage);
static void M_Control(int16_t item_num);
static void M_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
static void M_Draw(const ITEM *item);
static void M_AddWalkable(int16_t item_num);

static const OBJECT_BOUNDS *M_Bounds(void)
{
    return &m_MovableBlock_Bounds;
}

static bool M_IsAgainstFloor(const ITEM *const item)
{
    int16_t room_num = item->room_num;
    const SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    return sector->floor.tilt == 0 && sector->floor.height == item->pos.y;
}

static bool M_IsAgainstCeiling(const ITEM *const item)
{
    int16_t room_num = item->room_num;
    const SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    const SECTOR *const sky_sector =
        Room_GetSkySector(sector, item->pos.x, item->pos.z);
    return sky_sector->ceiling.tilt == 0
        && sky_sector->ceiling.height == item->pos.y - WALL_L;
}

static int16_t M_GetFloorHeight(
    const ITEM *const item, int32_t x, int32_t y, int32_t z, int16_t height)
{
    if (item->status == IS_INVISIBLE) {
        return height;
    }

    // TODO OG bug: camera and shadow behave like OG during push/pull.
    if (MovableBlock_IsPushPull(item)) {
        return height;
    }

    if (!M_IsItemOnTop(item, x, z)) {
        return height;
    }

    // If partially embedded from below e.g. jumping up into an overhead block.
    if (y <= item->pos.y && y > item->pos.y - WALL_L
        && M_IsAgainstCeiling(item)) {
        const SECTOR *const sector = Room_GetWorldSector(
            Room_Get(item->room_num), item->pos.x, item->pos.z);
        if (item->pos.y < sector->floor.height) {
            // If partially embedded from below e.g. jumping up into an overhead
            // block.
            return height;
        } else if (M_IsAgainstFloor(item)) {
            // Clamped between floor and ceiling. Match up with similar case in
            // M_GetCeilingHeight to return same sentinel value;
            return item->pos.y - WALL_L;
        }
    }

    // If under the bottom of the block.
    if (y > item->pos.y) {
        return height;
    }

    // If the the top of the block is under the floor height.
    if (item->pos.y - WALL_L >= height) {
        return height;
    }

    return item->pos.y - WALL_L;
}

static int16_t M_GetCeilingHeight(
    const ITEM *item, int32_t x, int32_t y, int32_t z, int16_t height)
{
    if (item->status == IS_INVISIBLE) {
        return height;
    }

    // TODO OG bug: camera and shadow behave like OG during push/pull.
    if (MovableBlock_IsPushPull(item)) {
        return height;
    }

    // Only care if we are inside the block footprint.
    if (!M_IsItemOnTop(item, x, z)) {
        return height;
    }

    if (y <= item->pos.y && y > item->pos.y - WALL_L && !item->gravity) {
        if (M_IsAgainstCeiling(item)) {
            // If clamped betwee floor and ceiling return same sentinel value as
            // M_GetFloorHeight.
            return M_IsAgainstFloor(item) ? item->pos.y - WALL_L : item->pos.y;
        }
        return height;
    }

    // If above the top of the block.
    if (y <= item->pos.y - WALL_L) {
        return height;
    }

    // If the the bottom of the block is above the ceiling height.
    if (item->pos.y <= height) {
        return height;
    }

    return item->pos.y;
}

static bool M_IsItemOnTop(
    const ITEM *const item, const int32_t x, const int32_t z)
{
    int32_t dx = x - item->pos.x;
    int32_t dz = z - item->pos.z;

    // Movable blocks' bounds don't match sector so estimate.
    return (dx >= -WALL_L / 2 && dx < WALL_L / 2)
        && (dz >= -WALL_L / 2 && dz < WALL_L / 2);
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

static bool M_TestCurrentSector(ITEM *item, int32_t block_height)
{
    int16_t room_num = item->room_num;
    const SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);

    // Check if there is a hard wall above.
    if (Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z)
        == NO_HEIGHT) {
        return true;
    }

    // Make sure there is nothing on top of the block.
    if (Room_GetHeight(
            sector, item->pos.x, item->pos.y - block_height, item->pos.z)
        != item->pos.y - block_height) {
        return false;
    }

    return true;
}

static bool M_TestPush(ITEM *item, int32_t block_height, DIRECTION quadrant)
{
    if (!M_TestCurrentSector(item, block_height)) {
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
    if (!M_TestCurrentSector(item, block_height)) {
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

    // Test block destination sector.
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

    // Test Lara destination sector.
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
    coll.radius = LARA_RADIUS;
    coll.quadrant = (quadrant + 2) & 3;
    if (Collide_CollideStaticObjects(&coll, x, y, z, room_num, LARA_HEIGHT)) {
        return false;
    }

    return true;
}

static bool M_TestDeathCollision(const ITEM *const item, const ITEM *const lara)
{
    return g_GameFlow.enable_killer_pushblocks
        && !g_Config.debug.enable_invulnerability && item->gravity
        && Lara_TestBoundsCollide(item, 0);
}

static bool M_TestEmbedCollision(const ITEM *const item, const ITEM *const lara)
{
    return M_IsItemOnTop(item, lara->pos.x, lara->pos.z)
        && lara->pos.y <= item->pos.y && lara->pos.y > item->pos.y - WALL_L
        && !item->gravity && !lara->gravity
        && item->current_anim_state == MOVABLE_BLOCK_STATE_STILL
        && lara->current_anim_state != LS_PULL_BLOCK
        && lara->current_anim_state != LS_PUSH_BLOCK;
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
    Item_SwitchToAnim(lara, LA_BOULDER_DEATH, 0);

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

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = MovableBlock_Initialise;
    obj->handle_save_func = M_HandleSave;
    obj->control_func = M_Control;
    obj->floor_height_func = M_GetFloorHeight;
    obj->ceiling_height_func = M_GetCeilingHeight;
    obj->draw_func = M_Draw;
    obj->collision_func = M_Collision;
    obj->save_position = true;
    obj->save_anim = true;
    obj->save_flags = true;
    obj->base_rot.y = true;
    obj->bounds_func = M_Bounds;
    obj->add_walkable_func = M_AddWalkable;
}

static void M_HandleSave(ITEM *const item, const SAVEGAME_STAGE stage)
{
    if (stage == SAVEGAME_STAGE_BEFORE_LOAD) {
        MovableBlock_UpdateBox(item, false);
        // Remember where walkable is in level file.
        MovableBlock_SetLinked(item);
    } else if (stage == SAVEGAME_STAGE_AFTER_LOAD) {
        const int16_t item_num = Item_GetIndex(item);
        if (item->flags & IF_KILLED) {
            Walkable_Remove(item_num);
            return;
        }
        if (item->status == IS_ACTIVE && !item->gravity
            && item->current_anim_state == MOVABLE_BLOCK_STATE_STILL) {
            Item_RemoveActive(Item_GetIndex(item));
            item->status = IS_INACTIVE;
        }
        // Reposition walkable to loaded position.
        const GAME_VECTOR target = {
            .pos = item->pos,
            .room_num = item->room_num,
        };
        Walkable_Reposition(item_num, MovableBlock_GetLinked(item), target);
        MovableBlock_SetLinked(item);
        MovableBlock_UpdateBox(item, true);
    }
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (item->status == IS_INVISIBLE) {
        return;
    }

    if (MovableBlock_GetGravityFrames(item) > 0) {
        MovableBlock_SetGravityFrames(
            item, MovableBlock_GetGravityFrames(item) - 1);
        return;
    }

    if (item->flags & IF_ONE_SHOT) {
        Item_Kill(item_num);
        Walkable_Remove(item_num);
        MovableBlock_UpdateBox(item, false);
        return;
    }

    Item_Animate(item);

    int16_t room_num = item->room_num;

    // Check if the block is floating, on a walkable, or on the pit floor.
    // ROUND_TO_HALF_CLICK because block can fall through floor to undefined
    // sector.
    const ROOM *const room = Room_Get(Room_GetIndexFromPos(
        item->pos.x, ROUND_TO_HALF_CLICK(item->pos.y), item->pos.z));
    const SECTOR *sector = Room_GetWorldSector(room, item->pos.x, item->pos.z);
    int32_t under_block_height = Room_GetHeightEx(
        sector, item->pos.x, item->pos.y, item->pos.z, false, item_num);
    const int32_t top_of_block_height =
        Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);
    const SECTOR *room_num_sector = Room_GetSector(
        item->pos.x, top_of_block_height, item->pos.z, &room_num);

    // Checks if a block is on or fell through a walkable item.
    // Gravity and fall speed greatly affect the block's y position and whether
    // it landed on another walkable. If this check is too early, gravity is
    // stopped one frame early. If this check happens after the block falls
    // through another walkable, under_block_height is set to the floor height
    // under the walkable that fell through. So the block's y position is
    // rounded to a half click to check if it landed on another walkable.
    const bool on_walkable = Room_IsOnWalkable(
        sector, item->pos.x, ROUND_TO_HALF_CLICK(item->pos.y), item->pos.z,
        ROUND_TO_HALF_CLICK(item->pos.y), item_num);
    if (on_walkable) {
        under_block_height = ROUND_TO_HALF_CLICK(item->pos.y);
    }

    if (item->pos.y < under_block_height && !MovableBlock_IsPushPull(item)) {
        // Block is activated and floating in the air.
        item->gravity = true;
    } else if (item->gravity) {
        // Block hits the ground or another walkable.
        item->gravity = false;
        item->fall_speed = 0;
        item->pos.y = under_block_height;
        item->status = IS_DEACTIVATED;
        ItemAction_Run(ITEM_ACTION_FLOOR_SHAKE, item);
        Sound_Effect(SFX_T_REX_FOOTSTOMP, &item->pos, SPM_NORMAL);
    } else if (
        // If block is at/under floor height, no gravity, and isn't being
        // pushed/pulled anymore. Prevents blocks from getting stuck in
        // IS_INACTIVE if retriggered.
        item->pos.y >= under_block_height && !item->gravity
        && !MovableBlock_IsPushPull(item)) {
        item->status = IS_INACTIVE;
        Item_RemoveActive(item_num);
    }

    // Don't update room number if on a walkable because room number can fall
    // through to a pit room (e.g. trapdoors).
    if (!on_walkable) {
        Item_UpdateRoom(item_num, room_num);
    }

    if (item->status == IS_DEACTIVATED) {
        const GAME_VECTOR target = {
            .pos = item->pos,
            .room_num = item->room_num,
        };
        Walkable_Reposition(item_num, MovableBlock_GetLinked(item), target);
        MovableBlock_SetLinked(item);
        item->status = IS_INACTIVE;
        Item_RemoveActive(item_num);
        MovableBlock_UpdateBox(item, true);
        Room_TestTriggers(item);
    }
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    const OBJECT *const obj = Object_Get(item->object_id);

    if (item->status == IS_INVISIBLE) {
        return;
    }

    if (M_TestDeathCollision(item, lara_item)) {
        M_KillLara(item, lara_item);
        return;
    }

    if (M_TestEmbedCollision(item, lara_item)) {
        lara_item->pos.y = item->pos.y - WALL_L;
    }

    if (item->current_anim_state == MOVABLE_BLOCK_STATE_STILL) {
        MovableBlock_SetPushPull(item, false);
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
            MovableBlock_UpdateRotation(item, 0);
            break;
        case DIR_EAST:
            MovableBlock_UpdateRotation(item, DEG_90);
            break;
        case DIR_SOUTH:
            MovableBlock_UpdateRotation(item, -DEG_180);
            break;
        case DIR_WEST:
            MovableBlock_UpdateRotation(item, -DEG_90);
            break;
        default:
            break;
        }

        if (!Lara_TestPosition(item, obj->bounds_func())) {
            return;
        }

        // OG fix: stop pushing blocks through doors
        if (M_TestDoor(lara_item, coll)) {
            return;
        }

        switch (quadrant) {
        case DIR_NORTH:
            lara_item->pos.z &= -WALL_L;
            lara_item->pos.z += WALL_L - LARA_RADIUS;
            break;
        case DIR_SOUTH:
            lara_item->pos.z &= -WALL_L;
            lara_item->pos.z += LARA_RADIUS;
            break;
        case DIR_EAST:
            lara_item->pos.x &= -WALL_L;
            lara_item->pos.x += WALL_L - LARA_RADIUS;
            break;
        case DIR_WEST:
            lara_item->pos.x &= -WALL_L;
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
    } else if (Item_TestAnimEqual(lara_item, LA_PUSHABLE_GRAB)) {
        if (!Item_TestFrameEqual(lara_item, LF_PPREADY)) {
            return;
        }

        if (!Lara_TestPosition(item, obj->bounds_func())) {
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

        MovableBlock_SetLinked(item);
        item->status = IS_ACTIVE;
        Item_AddActive(item_num);
        MovableBlock_UpdateBox(item, false);
        Item_Animate(item);
        Lara_Animate(lara_item);
        MovableBlock_SetPushPull(item, true);
    }
}

static void M_Draw(const ITEM *const item)
{
    if (item->status == IS_ACTIVE) {
        Object_DrawUnclippedItem(item);
    } else {
        Object_DrawAnimatingItem(item);
    }
}

static void M_AddWalkable(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    Walkable_Add(item_num, item->pos);
}

REGISTER_OBJECT(O_MOVABLE_BLOCK_1, M_Setup)
REGISTER_OBJECT(O_MOVABLE_BLOCK_2, M_Setup)
REGISTER_OBJECT(O_MOVABLE_BLOCK_3, M_Setup)
REGISTER_OBJECT(O_MOVABLE_BLOCK_4, M_Setup)
