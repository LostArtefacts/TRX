#include "game/objects/general/bridge_common.h"

static int16_t M_GetFloorHeight(
    const ITEM *item, int32_t x, int32_t y, int32_t z, int16_t height);
static int16_t M_GetCeilingHeight(
    const ITEM *item, int32_t x, int32_t y, int32_t z, int16_t height);
static void M_Setup(OBJECT *obj);

static int16_t M_GetFloorHeight(
    const ITEM *const item, const int32_t x, const int32_t y, const int32_t z,
    const int16_t height)
{
    if (y > item->pos.y) {
        return height;
    }
    return item->pos.y;
}

static int16_t M_GetCeilingHeight(
    const ITEM *const item, const int32_t x, const int32_t y, const int32_t z,
    const int16_t height)
{
    if (y <= item->pos.y) {
        return height;
    }
    return item->pos.y + STEP_L;
}

static void M_Setup(OBJECT *const obj)
{
    obj->floor_height_func = M_GetFloorHeight;
    obj->ceiling_height_func = M_GetCeilingHeight;
}

REGISTER_OBJECT(O_BRIDGE_FLAT, M_Setup)
