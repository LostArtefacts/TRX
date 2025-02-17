#include "game/objects/general/bridge_flat.h"

#include "game/objects/general/bridge_common.h"

#include <libtrx/config.h>

static void M_Initialise(int16_t item_num);
static int16_t M_GetFloorHeight(
    const ITEM *item, int32_t x, int32_t y, int32_t z, int16_t height);
static int16_t M_GetCeilingHeight(
    const ITEM *item, int32_t x, int32_t y, int32_t z, int16_t height);

static void M_Initialise(const int16_t item_num)
{
    Bridge_FixEmbeddedPosition(item_num);
}

static int16_t M_GetFloorHeight(
    const ITEM *item, const int32_t x, const int32_t y, const int32_t z,
    const int16_t height)
{
    if (g_Config.gameplay.fix_bridge_collision
        && !Bridge_IsSameSector(x, z, item)) {
        return height;
    }

    if (y > item->pos.y) {
        return height;
    }

    if (g_Config.gameplay.fix_bridge_collision && item->pos.y >= height) {
        return height;
    }

    return item->pos.y;
}

static int16_t M_GetCeilingHeight(
    const ITEM *item, const int32_t x, const int32_t y, const int32_t z,
    const int16_t height)
{
    if (g_Config.gameplay.fix_bridge_collision
        && !Bridge_IsSameSector(x, z, item)) {
        return height;
    }

    if (y <= item->pos.y) {
        return height;
    }

    if (g_Config.gameplay.fix_bridge_collision && item->pos.y <= height) {
        return height;
    }

    return item->pos.y + STEP_L;
}

void BridgeFlat_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->floor_height_func = M_GetFloorHeight;
    obj->ceiling_height_func = M_GetCeilingHeight;
}
