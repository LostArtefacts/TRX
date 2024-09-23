#include "game/objects/general/bridge_tilt1.h"

#include "config.h"
#include "game/objects/general/bridge_common.h"

static void M_Initialise(int16_t item_num);
static int16_t M_GetFloorHeight(
    const ITEM *item, int32_t x, int32_t y, int32_t z, int16_t height);
static int16_t M_GetCeilingHeight(
    const ITEM *item, int32_t x, int32_t y, int32_t z, int16_t height);

static void M_Initialise(const int16_t item_num)
{
    Bridge_FixEmbeddedPosition(item_num);
}

int16_t M_GetFloorHeight(
    const ITEM *item, const int32_t x, const int32_t y, const int32_t z,
    const int16_t height)
{
    if (g_Config.fix_bridge_collision && !Bridge_IsSameSector(x, z, item)) {
        return height;
    }

    const int32_t offset_height =
        item->pos.y + (Bridge_GetOffset(item, x, y, z) / 4);
    if (y > offset_height || item->pos.y >= height) {
        return height;
    }

    if (g_Config.fix_bridge_collision && item->pos.y >= height) {
        return height;
    }

    return offset_height;
}

int16_t M_GetCeilingHeight(
    const ITEM *item, const int32_t x, const int32_t y, const int32_t z,
    const int16_t height)
{
    if (g_Config.fix_bridge_collision && !Bridge_IsSameSector(x, z, item)) {
        return height;
    }

    const int32_t offset_height =
        item->pos.y + (Bridge_GetOffset(item, x, y, z) / 4);
    if (y <= offset_height) {
        return height;
    }

    if (g_Config.fix_bridge_collision && item->pos.y <= height) {
        return height;
    }

    return offset_height + STEP_L;
}

void BridgeTilt1_Setup(OBJECT *obj)
{
    obj->initialise = M_Initialise;
    obj->floor_height_func = M_GetFloorHeight;
    obj->ceiling_height_func = M_GetCeilingHeight;
}
