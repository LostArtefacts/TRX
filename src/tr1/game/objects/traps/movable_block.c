#include "game/game_flow.h"
#include "game/input.h"
#include "game/item_actions.h"
#include "game/lara.h"
#include "game/objects/common.h"
#include "game/objects/vars.h"
#include "game/random.h"
#include "game/sound.h"
#include "game/spawn.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/collision.h>
#include <libtrx/game/lara/const.h>
#include <libtrx/game/objects/traps/movable_block.h>
#include <libtrx/log.h>
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
static int16_t M_GetFloorHeight(
    const ITEM *item, int32_t x, int32_t y, int32_t z, int16_t height);
static int16_t M_GetCeilingHeight(
    const ITEM *item, int32_t x, int32_t y, int32_t z, int16_t height);
static bool M_IsItemOnTop(const ITEM *item, int32_t x, int32_t z);
static bool M_TestDoor(ITEM *lara_item, COLL_INFO *coll);
static bool M_TestDestination(ITEM *item, int32_t block_height);
static bool M_TestPush(ITEM *item, int32_t block_height, DIRECTION quadrant);
static bool M_TestPull(ITEM *item, int32_t block_height, DIRECTION quadrant);
static bool M_TestDeathCollision(ITEM *item, const ITEM *lara);
static void M_KillLara(const ITEM *item, ITEM *lara);
static void M_Setup(OBJECT *obj);
static void M_HandleSave(ITEM *item, SAVEGAME_STAGE stage);
static void M_Control(int16_t item_num);
static void M_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
static void M_Draw(const ITEM *item);

static const OBJECT_BOUNDS *M_Bounds(void)
{
    return &m_MovableBlock_Bounds;
}

static int16_t M_GetFloorHeight(
    const ITEM *const item, int32_t x, int32_t y, int32_t z, int16_t height)
{
    if (item->status == IS_INVISIBLE) {
        return height;
    }

    // TODO Makes camera and shadow behave like OG during push/pull.
    // Still on top of block at end of a pull like OG.
    if (MovableBlock_IsPushPull(item)) {
        return height;
    }

    // Only care if we are inside the block footprint.
    if (!M_IsItemOnTop(item, x, z)) {
        return height;
    }

    // If inside the block.
    if (y <= item->pos.y && y > item->pos.y - WALL_L) {
        return item->pos.y - WALL_L;
    }

    // If under the bottom of the block.
    if (y > item->pos.y) {
        return height;
    }

    // If the the top of the block is under the floor height.
    if (item->pos.y - WALL_L >= height) {
        return height;
    }

    // LOG_DEBUG(
    //     "Raise floor for block. block xyz: %d %d %d; height: %d; xyz: "
    //     "%d %d %d",
    //     item->pos.x, item->pos.y, item->pos.z, height, x, y, z);

    return item->pos.y - WALL_L;
}

static int16_t M_GetCeilingHeight(
    const ITEM *item, int32_t x, int32_t y, int32_t z, int16_t height)
{
    if (item->status == IS_INVISIBLE) {
        return height;
    }

    // TODO Makes camera and shadow behave like OG during push/pull.
    if (MovableBlock_IsPushPull(item)) {
        return height;
    }

    // Only care if we are inside the block footprint.
    if (!M_IsItemOnTop(item, x, z)) {
        return height;
    }

    // If inside the block.
    if (y <= item->pos.y && y > item->pos.y - WALL_L) {
        // return item->pos.y - WALL_L;
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

    // LOG_DEBUG(
    //     "Raise ceiling for block. block xyz: %d %d %d; height: %d; xyz: "
    //     "%d %d %d",
    //     item->pos.x, item->pos.y, item->pos.z, height, x, y, z);

    return item->pos.y;
}

static bool M_IsItemOnTop(
    const ITEM *const item, const int32_t x, const int32_t z)
{
    const BOUNDS_16 *const bounds = &Item_GetBestFrame(item)->bounds;
    if (bounds == nullptr) {
        return false;
    }

    const ITEM *const lara_item = Lara_GetItem();
    // Creature setups run before Lara is initialized.
    if (lara_item == nullptr) {
        return false;
    }

    int32_t dx = x - item->pos.x;
    int32_t dz = z - item->pos.z;

    DIRECTION quadrant = ((uint16_t)lara_item->rot.y + DEG_45) / DEG_90;
    switch (quadrant) {
    case DIR_NORTH:
        dx = -dx;
        dz = -dz;
        break;
    case DIR_EAST:
        int32_t t1 = dx;
        dx = dz;
        dz = -t1;
        break;
    case DIR_SOUTH:
        break;
    case DIR_WEST:
        int32_t t2 = dx;
        dx = -dz;
        dz = t2;
        break;
    default:
        break;
    }

    return dx >= bounds->min.x && dx <= bounds->max.x && dz >= bounds->min.z
        && dz <= bounds->max.z;
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
    // Check if there is a wall above.
    if (Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z)
        == NO_HEIGHT) {
        return true;
    }

    // Make sure floor above block has nothing on it.
    if (Room_GetHeight(
            sector, item->pos.x, item->pos.y - block_height, item->pos.z)
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
    // Make sure current sector is safe.
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

    // Check sector block will be pulled to.
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
    coll.radius = LARA_RADIUS;
    coll.quadrant = (quadrant + 2) & 3;
    if (Collide_CollideStaticObjects(&coll, x, y, z, room_num, LARA_HEIGHT)) {
        return false;
    }

    return true;
}

static bool M_TestDeathCollision(ITEM *const item, const ITEM *const lara)
{
    return g_GameFlow.enable_killer_pushblocks
        && !g_Config.debug.enable_invulnerability && item->gravity
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
}

static void M_HandleSave(ITEM *const item, const SAVEGAME_STAGE stage)
{
    if (stage == SAVEGAME_STAGE_AFTER_LOAD) {
        if (item->flags & IF_KILLED) {
            const int16_t item_num = Item_GetIndex(item);
            Item_RemoveWalkable(item_num);
        }
        if (item->status == IS_ACTIVE && !item->gravity
            && item->current_anim_state == MOVABLE_BLOCK_STATE_STILL) {
            Item_RemoveActive(Item_GetIndex(item));
            item->status = IS_INACTIVE;
        }
        const bool is_push_pull = item->status == IS_ACTIVE ? true : false;
        MovableBlock_SetPushPull(item, is_push_pull);
        MovableBlock_UpdateRotation(item, item->rot.y);
    }
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (item->status == IS_INVISIBLE) {
        return;
    }

    if (MovableBlock_GetGravityFrames(item) > 0) {
        LOG_DEBUG(
            "DELAY item_num: %d; gravity frames: %d", item_num,
            MovableBlock_GetGravityFrames(item) - 1);
        MovableBlock_SetGravityFrames(
            item, MovableBlock_GetGravityFrames(item) - 1);
        return;
    }

    if (item->flags & IF_ONE_SHOT) {
        Item_Kill(item_num);
        Item_RemoveWalkable(item_num);
        Item_SortWalkables();
        // TODO Fix enemy boxes.
        // Room_AlterFloorHeight(item, WALL_L);
        return;
    }

    Item_Animate(item);

    int16_t room_num = item->room_num;
    LOG_DEBUG(
        "initial item_num: %d; item->room_num: %d", item_num, item->room_num);

    // Check if the block is floating, on a walkable, or on the pit floor.
    // Gets the put room number which can break behavior.
    const ROOM *const room = Room_Get(room_num);
    const SECTOR *sector = Room_GetWorldSector(room, item->pos.x, item->pos.z);
    const int32_t top_of_block_height =
        Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);
    int32_t under_block_height = Room_GetHeightIgnore(
        sector, item->pos.x, item->pos.y, item->pos.z, false, item_num);
    const SECTOR *room_num_sector = Room_GetSector(
        item->pos.x, top_of_block_height, item->pos.z, &room_num);

    LOG_DEBUG("OG under_block_height: %d", under_block_height);
    // Checks if the block fell through a walkable. Can't check before it
    // happens because it will stop gravity one frame early. Can't nicely check
    // after it falls through because under_block_height will then be the floor
    // under the walkable that fell through.
    const bool rounded_on_walkable = Room_IsOnWalkable(
        sector, item->pos.x, ROUND_TO_HALF_CLICK(item->pos.y), item->pos.z,
        ROUND_TO_HALF_CLICK(item->pos.y), item_num);
    if (rounded_on_walkable) {
        under_block_height = ROUND_TO_HALF_CLICK(item->pos.y);
    }

    LOG_DEBUG(
        "status: %d; gravity: %d; pos.y: %d; top_of_block_height: %d, "
        "under_block_height: %d; room_num: %d; "
        "ROUND_TO_HALF_CLICK(item->pos.y): "
        "%d; rounded_on_walkable: %d; push/pull: %d; gravity frames: %d; "
        "fall_speed: %d",
        item->status, item->gravity, item->pos.y, top_of_block_height,
        under_block_height, room_num, ROUND_TO_HALF_CLICK(item->pos.y),
        rounded_on_walkable, MovableBlock_IsPushPull(item),
        MovableBlock_GetGravityFrames(item), item->fall_speed);

    // Don't continue gravity if on a walkable.
    // But walkable is affected by falling speed and bridges
    // can have non click heights.
    // ROUND_TO_CLICK_UP(item->pos.y) != under_block_height fixes blocks
    // falling through bridges/trapdoors. BUT turns off gravity a frame early
    // which breaks a death block just above Lara's head.
    // && ROUND_TO_CLICK_UP(item->pos.y) != under_block_height
    if (item->pos.y < under_block_height && !MovableBlock_IsPushPull(item)) {
        // Start falling because the block is floating in the air.
        LOG_DEBUG("Start gravity!");
        item->gravity = true;
    } else if (item->gravity) {
        // The block hits the ground or walkable.
        LOG_DEBUG("Stop gravity!");
        item->gravity = false;
        item->pos.y = under_block_height;
        item->status = IS_DEACTIVATED;
        ItemAction_Run(ITEM_ACTION_FLOOR_SHAKE, item);
        Sound_Effect(SFX_T_REX_FOOTSTOMP, &item->pos, SPM_NORMAL);
    } else if (
        // If block is at/under floor height, no gravity, and isn't being
        // pushed/pulled anymore.
        // Prevents blocks from getting stuck in IS_INACTIVE if retriggered.
        item->pos.y >= under_block_height && !item->gravity
        && !MovableBlock_IsPushPull(item)) {
        LOG_DEBUG("Remove active bc of height!");
        item->status = IS_INACTIVE;
        Item_RemoveActive(item_num);
    }

    // Don't update room number if on a walkable because
    // room number can fall through trapdoors to a pit room.
    if (!rounded_on_walkable) {
        Item_UpdateRoom(item_num, room_num);
    }

    if (item->status == IS_DEACTIVATED) {
        item->status = IS_INACTIVE;
        Item_RemoveActive(item_num);
        // TODO Fix enemy boxes.
        // Room_AlterFloorHeight(item, -WALL_L);
        Item_SortWalkables();
        Room_TestTriggers(item);
        LOG_DEBUG("Remove active bc IS_DEACTIVATED!");
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

        item->status = IS_ACTIVE;
        Item_AddActive(item_num);
        // TODO Fix enemy boxes.
        // Room_AlterFloorHeight(item, WALL_L);
        Item_Animate(item);
        Lara_Animate(lara_item);
        MovableBlock_SetPushPull(item, true);
        LOG_DEBUG("Start push/pull item_num: %d", item_num);
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

REGISTER_OBJECT(O_MOVABLE_BLOCK_1, M_Setup)
REGISTER_OBJECT(O_MOVABLE_BLOCK_2, M_Setup)
REGISTER_OBJECT(O_MOVABLE_BLOCK_3, M_Setup)
REGISTER_OBJECT(O_MOVABLE_BLOCK_4, M_Setup)
