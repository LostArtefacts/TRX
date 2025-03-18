#include "game/objects/general/door.h"
#include "game/objects/general/general.h"

typedef enum {
    DRAWBRIDGE_STATE_CLOSED = DOOR_STATE_CLOSED,
    DRAWBRIDGE_STATE_OPEN = DOOR_STATE_OPEN,
} DRAWBRIDGE_STATE;

static int16_t M_GetFloorHeight(
    const ITEM *item, int32_t x, int32_t y, int32_t z, int16_t height);
static int16_t M_GetCeilingHeight(
    const ITEM *item, int32_t x, int32_t y, int32_t z, int16_t height);
static bool M_IsItemOnTop(const ITEM *item, int32_t z, int32_t x);
static void M_Setup(OBJECT *obj);
static void M_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);

static int16_t M_GetFloorHeight(
    const ITEM *const item, const int32_t x, const int32_t y, const int32_t z,
    const int16_t height)
{
    if (item->current_anim_state != DRAWBRIDGE_STATE_OPEN) {
        return height;
    } else if (!M_IsItemOnTop(item, z, x)) {
        return height;
    } else if (item->pos.y < y) {
        return height;
    }
    return item->pos.y;
}

static int16_t M_GetCeilingHeight(
    const ITEM *const item, const int32_t x, const int32_t y, const int32_t z,
    const int16_t height)
{
    if (item->current_anim_state != DRAWBRIDGE_STATE_OPEN) {
        return height;
    } else if (!M_IsItemOnTop(item, z, x)) {
        return height;
    } else if (item->pos.y >= y) {
        return height;
    }
    return item->pos.y + STEP_L;
}

static bool M_IsItemOnTop(
    const ITEM *const item, const int32_t z, const int32_t x)
{
    // drawbridge sector
    const XZ_32 obj = {
        .x = item->pos.x >> WALL_SHIFT,
        .z = item->pos.z >> WALL_SHIFT,
    };

    // test sector
    const XZ_32 test = {
        .x = x >> WALL_SHIFT,
        .z = z >> WALL_SHIFT,
    };

    switch (item->rot.y) {
    case 0:
        return test.x == obj.x && (test.z == obj.z - 1 || test.z == obj.z - 2);

    case -DEG_180:
        return test.x == obj.x && (test.z == obj.z + 1 || test.z == obj.z + 2);

    case -DEG_90:
        return test.z == obj.z && (test.x == obj.x + 1 || test.x == obj.x + 2);

    case DEG_90:
        return test.z == obj.z && (test.x == obj.x - 1 || test.x == obj.x - 2);
    }

    return false;
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->control_func = General_Control;
    obj->collision_func = M_Collision;
    obj->floor_height_func = M_GetFloorHeight;
    obj->ceiling_height_func = M_GetCeilingHeight;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    const ITEM *const item = Item_Get(item_num);
    if (item->current_anim_state == DRAWBRIDGE_STATE_CLOSED) {
        Door_Collision(item_num, lara_item, coll);
    }
}

REGISTER_OBJECT(O_DRAWBRIDGE, M_Setup)
