#include "game/objects/general/bridge_tilt1.h"

#include "config.h"
#include "game/objects/general/bridge_common.h"

static void BridgeTilt1_Initialise(int16_t item_num);
static int16_t BridgeTilt1_GetFloorHeight(
    const ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t height);
static int16_t BridgeTilt1_GetCeilingHeight(
    const ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t height);

static void BridgeTilt1_Initialise(const int16_t item_num)
{
    Bridge_FixEmbeddedPosition(item_num);
}

int16_t BridgeTilt1_GetFloorHeight(
    const ITEM_INFO *item, const int32_t x, const int32_t y, const int32_t z,
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

int16_t BridgeTilt1_GetCeilingHeight(
    const ITEM_INFO *item, const int32_t x, const int32_t y, const int32_t z,
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

void BridgeTilt1_Setup(OBJECT_INFO *obj)
{
    obj->initialise = BridgeTilt1_Initialise;
    obj->floor_height_func = BridgeTilt1_GetFloorHeight;
    obj->ceiling_height_func = BridgeTilt1_GetCeilingHeight;
}
