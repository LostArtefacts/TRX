#include "game/objects/general/lift.h"

#include "game/items.h"
#include "game/room.h"
#include "global/vars.h"

#include <libtrx/game/game_buf.h>

#define LIFT_WAIT_TIME (3 * FRAMES_PER_SECOND) // = 90
#define LIFT_SHIFT 16
#define LIFT_HEIGHT (STEP_L * 5) // = 1280
#define LIFT_TRAVEL_DIST (STEP_L * 22)

typedef enum {
    LIFT_STATE_DOOR_CLOSED = 0,
    LIFT_STATE_DOOR_OPEN = 1,
} LIFT_STATE;

static void M_FloorCeiling(
    const ITEM *item, int32_t x, int32_t y, int32_t z, int32_t *out_floor,
    int32_t *out_ceiling);
static int16_t M_GetFloorHeight(
    const ITEM *item, int32_t x, int32_t y, int32_t z, int16_t height);
static int16_t M_GetCeilingHeight(
    const ITEM *item, int32_t x, int32_t y, int32_t z, int16_t height);
static void M_Setup(OBJECT *obj);
static void M_Initialise(int16_t item_num);
static void M_Control(int16_t item_num);

static void M_FloorCeiling(
    const ITEM *const item, const int32_t x, const int32_t y, const int32_t z,
    int32_t *const out_floor, int32_t *const out_ceiling)
{
    const XZ_32 lift_tile = {
        .x = item->pos.x >> WALL_SHIFT,
        .z = item->pos.z >> WALL_SHIFT,
    };

    const XZ_32 lara_tile = {
        .x = g_LaraItem->pos.x >> WALL_SHIFT,
        .z = g_LaraItem->pos.z >> WALL_SHIFT,
    };

    const XZ_32 test_tile = {
        .x = x >> WALL_SHIFT,
        .z = z >> WALL_SHIFT,
    };

    // clang-format off
    const bool point_in_shaft =
        (test_tile.x == lift_tile.x || test_tile.x + 1 == lift_tile.x) &&
        (test_tile.z == lift_tile.z || test_tile.z - 1 == lift_tile.z);

    const bool lara_in_shaft =
        (lara_tile.x == lift_tile.x || lara_tile.x + 1 == lift_tile.x) &&
        (lara_tile.z == lift_tile.z || lara_tile.z - 1 == lift_tile.z);
    // clang-format on

    const int32_t lift_floor = item->pos.y;
    const int32_t lift_ceiling = item->pos.y - LIFT_HEIGHT;

    *out_floor = 0x7FFF;
    *out_ceiling = -0x7FFF;

    if (lara_in_shaft) {
        if (item->current_anim_state == LIFT_STATE_DOOR_CLOSED
            && g_LaraItem->pos.y < lift_floor + STEP_L
            && g_LaraItem->pos.y > lift_ceiling + STEP_L) {
            if (point_in_shaft) {
                *out_floor = lift_floor;
                *out_ceiling = lift_ceiling + STEP_L;
            } else {
                *out_floor = NO_HEIGHT;
                *out_ceiling = 0x7FFF;
            }
        } else if (point_in_shaft) {
            if (g_LaraItem->pos.y < lift_ceiling + STEP_L) {
                *out_floor = lift_ceiling;
            } else if (g_LaraItem->pos.y < lift_floor + STEP_L) {
                *out_floor = lift_floor;
                *out_ceiling = lift_ceiling + STEP_L;
            } else {
                *out_ceiling = lift_floor + STEP_L;
            }
        }
    } else if (point_in_shaft) {
        if (y <= lift_ceiling) {
            *out_floor = lift_ceiling;
        } else if (y >= lift_floor + STEP_L) {
            *out_ceiling = lift_floor + STEP_L;
        } else if (item->current_anim_state == LIFT_STATE_DOOR_OPEN) {
            *out_floor = lift_floor;
            *out_ceiling = lift_ceiling + STEP_L;
        } else {
            *out_floor = NO_HEIGHT;
            *out_ceiling = 0x7FFF;
        }
    }
}

static int16_t M_GetFloorHeight(
    const ITEM *const item, const int32_t x, const int32_t y, const int32_t z,
    const int16_t height)
{
    int32_t new_floor;
    int32_t new_height;
    M_FloorCeiling(item, x, y, z, &new_floor, &new_height);
    if (new_floor >= height) {
        return height;
    }
    return new_floor;
}

static int16_t M_GetCeilingHeight(
    const ITEM *const item, const int32_t x, const int32_t y, const int32_t z,
    const int16_t height)
{
    int32_t new_floor;
    int32_t new_height;
    M_FloorCeiling(item, x, y, z, &new_floor, &new_height);
    if (new_height <= height) {
        return height;
    }
    return new_height;
}

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->floor_height_func = M_GetFloorHeight;
    obj->ceiling_height_func = M_GetCeilingHeight;
    obj->save_position = 1;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    LIFT_INFO *const lift_data =
        GameBuf_Alloc(sizeof(LIFT_INFO), GBUF_ITEM_DATA);
    lift_data->start_height = item->pos.y;
    lift_data->wait_time = 0;

    item->data = lift_data;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    LIFT_INFO *const lift_data = item->data;

    if (Item_IsTriggerActive(item)) {
        if (item->pos.y
            < lift_data->start_height + LIFT_TRAVEL_DIST - LIFT_SHIFT) {
            if (lift_data->wait_time < LIFT_WAIT_TIME) {
                item->goal_anim_state = LIFT_STATE_DOOR_OPEN;
                lift_data->wait_time++;
            } else {
                item->goal_anim_state = LIFT_STATE_DOOR_CLOSED;
                item->pos.y += LIFT_SHIFT;
            }
        } else {
            item->goal_anim_state = LIFT_STATE_DOOR_OPEN;
            lift_data->wait_time = 0;
        }
    } else {
        if (item->pos.y > lift_data->start_height + LIFT_SHIFT) {
            if (lift_data->wait_time < LIFT_WAIT_TIME) {
                item->goal_anim_state = LIFT_STATE_DOOR_OPEN;
                lift_data->wait_time++;
            } else {
                item->goal_anim_state = LIFT_STATE_DOOR_CLOSED;
                item->pos.y -= LIFT_SHIFT;
            }
        } else {
            item->goal_anim_state = LIFT_STATE_DOOR_OPEN;
            lift_data->wait_time = 0;
        }
    }

    Item_Animate(item);

    int16_t room_num = item->room_num;
    Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    if (item->room_num != room_num) {
        Item_NewRoom(item_num, room_num);
    }
}

REGISTER_OBJECT(O_LIFT, M_Setup)
