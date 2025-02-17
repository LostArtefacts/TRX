#include "game/objects/general/bridge_tilt_1.h"

#include "game/objects/general/bridge_common.h"

static int16_t M_GetFloorHeight(
    const ITEM *item, int32_t x, int32_t y, int32_t z, int16_t height);
static int16_t M_GetCeilingHeight(
    const ITEM *item, int32_t x, int32_t y, int32_t z, int16_t height);

static int16_t M_GetFloorHeight(
    const ITEM *const item, const int32_t x, const int32_t y, const int32_t z,
    const int16_t height)
{
    const int32_t offset_height =
        item->pos.y + (Bridge_GetOffset(item, x, z) / 4);
    if (y > offset_height) {
        return height;
    }
    return offset_height;
}

static int16_t M_GetCeilingHeight(
    const ITEM *const item, const int32_t x, const int32_t y, const int32_t z,
    const int16_t height)
{
    const int32_t offset_height =
        item->pos.y + (Bridge_GetOffset(item, x, z) / 4);
    if (y <= offset_height) {
        return height;
    }
    return offset_height + STEP_L;
}

void BridgeTilt1_Setup(void)
{
    OBJECT *const obj = Object_Get(O_BRIDGE_TILT_1);
    obj->floor_height_func = M_GetFloorHeight;
    obj->ceiling_height_func = M_GetCeilingHeight;
}
