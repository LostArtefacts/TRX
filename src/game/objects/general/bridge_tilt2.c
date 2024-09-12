#include "config.h"
#include "game/objects/general/bridge_common.h"
#include "game/objects/general/bridge_tilt1.h"

static void BridgeTilt2_Initialise(int16_t item_num);
static int16_t BridgeTilt2_GetFloorHeight(
    const ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t height);
static int16_t BridgeTilt2_GetCeilingHeight(
    const ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t height);

static void BridgeTilt2_Initialise(const int16_t item_num)
{
    Bridge_FixEmbeddedPosition(item_num);
}

int16_t BridgeTilt2_GetFloorHeight(
    const ITEM_INFO *item, const int32_t x, const int32_t y, const int32_t z,
    const int16_t height)
{
    if (g_Config.fix_bridge_collision && !Bridge_IsSameSector(x, z, item)) {
        return height;
    }

    const int32_t offset_height =
        item->pos.y + (Bridge_GetOffset(item, x, y, z) / 2);
    if (y > offset_height) {
        return height;
    }

    if (g_Config.fix_bridge_collision && item->pos.y >= height) {
        return height;
    }

    return offset_height;
}

int16_t BridgeTilt2_GetCeilingHeight(
    const ITEM_INFO *item, const int32_t x, const int32_t y, const int32_t z,
    const int16_t height)
{
    if (g_Config.fix_bridge_collision && !Bridge_IsSameSector(x, z, item)) {
        return height;
    }

    const int32_t offset_height =
        item->pos.y + (Bridge_GetOffset(item, x, y, z) / 2);
    if (y <= offset_height) {
        return height;
    }

    if (g_Config.fix_bridge_collision && item->pos.y <= height) {
        return height;
    }

    return offset_height + STEP_L;
}

void BridgeTilt2_Setup(OBJECT_INFO *obj)
{
    obj->initialise = BridgeTilt2_Initialise;
    obj->floor_height_func = BridgeTilt2_GetFloorHeight;
    obj->ceiling_height_func = BridgeTilt2_GetCeilingHeight;
}
