#include "game/objects/bridge.h"

#include "game/collide.h"
#include "game/draw.h"
#include "game/objects/cog.h"
#include "global/vars.h"
#include "src/config.h"
#include "log.h"

bool isSameSector(ITEM_INFO *item, int32_t x, int32_t y, int32_t z)
{
    int32_t a_Sector_x = x / WALL_L;
    int32_t a_Sector_z = z / WALL_L;
    int32_t b_Sector_x = item->pos.x / WALL_L;
    int32_t b_Sector_z = item->pos.z / WALL_L;

    // LOG_DEBUG("a_Sector_x: %d", a_Sector_x);
    // LOG_DEBUG("b_Sector_x: %d", a_Sector_x);
    // LOG_DEBUG("a_Sector_z: %d", a_Sector_x);
    // LOG_DEBUG("b_Sector_z: %d", a_Sector_x);

    return a_Sector_x == b_Sector_x && a_Sector_z == b_Sector_z;
}

void SetupBridgeFlat(OBJECT_INFO *obj)
{
    obj->floor = BridgeFlatFloor;
    obj->ceiling = BridgeFlatCeiling;
}

void SetupBridgeTilt1(OBJECT_INFO *obj)
{
    obj->floor = BridgeTilt1Floor;
    obj->ceiling = BridgeTilt1Ceiling;
}

void SetupBridgeTilt2(OBJECT_INFO *obj)
{
    obj->floor = BridgeTilt2Floor;
    obj->ceiling = BridgeTilt2Ceiling;
}

void SetupDrawBridge(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->ceiling = DrawBridgeCeiling;
    obj->collision = DrawBridgeCollision;
    obj->control = CogControl;
    obj->save_anim = 1;
    obj->save_flags = 1;
    obj->floor = DrawBridgeFloor;
}

int32_t OnDrawBridge(ITEM_INFO *item, int32_t x, int32_t y)
{
    int32_t ix = item->pos.z >> WALL_SHIFT;
    int32_t iy = item->pos.x >> WALL_SHIFT;

    x >>= WALL_SHIFT;
    y >>= WALL_SHIFT;

    if (item->pos.y_rot == 0 && y == iy && (x == ix - 1 || x == ix - 2)) {
        return 1;
    }
    if (item->pos.y_rot == -PHD_180 && y == iy
        && (x == ix + 1 || x == ix + 2)) {
        return 1;
    }
    if (item->pos.y_rot == PHD_90 && x == ix && (y == iy - 1 || y == iy - 2)) {
        return 1;
    }
    if (item->pos.y_rot == -PHD_90 && x == ix && (y == iy + 1 || y == iy + 2)) {
        return 1;
    }

    return 0;
}

void DrawBridgeFloor(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height)
{
    if (item->current_anim_state != DOOR_OPEN) {
        return;
    }
    if (!OnDrawBridge(item, z, x)) {
        return;
    }

    if (y <= item->pos.y) {
        *height = item->pos.y;
    }
}

void DrawBridgeCeiling(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height)
{
    if (item->current_anim_state != DOOR_OPEN) {
        return;
    }
    if (!OnDrawBridge(item, z, x)) {
        return;
    }

    if (y > item->pos.y) {
        *height = item->pos.y + STEP_L;
    }
}

void DrawBridgeCollision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &g_Items[item_num];
    if (item->current_anim_state == DOOR_CLOSED) {
        DoorCollision(item_num, lara_item, coll);
    }
}

void BridgeFlatFloor(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height)
{
    if (g_Config.fix_grab) {
        if (item->pos.y >= y)
        {
            *height = item->pos.y;
            // height_type = WALL;
            // OnObject = 1;
        }
        // if (!isSameSector(item, x, y, z)) {
        //     LOG_DEBUG("BridgeFlatFloor !isSameSector");
        //     return;
        // }

        // if (y > item->pos.y) {
        //     LOG_DEBUG("BridgeFlatFloor y > item->pos.y");
        //     return;
        // }

        // LOG_DEBUG("BridgeFlatFloor item->pos.y: %d", item->pos.y);
        // *height = item->pos.y;
    } else {
        if (y <= item->pos.y) {
            *height = item->pos.y;
        }
    }
}

void BridgeFlatCeiling(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height)
{
    // if (g_Config.fix_grab) {
    //     if (!isSameSector(item, x, y, z)) {
    //         LOG_DEBUG("BridgeFlatCeiling !isSameSector");
    //         return;
    //     }
    // }

    if (y > item->pos.y) {
        *height = item->pos.y + STEP_L;
    }
}

int32_t GetOffset(ITEM_INFO *item, int32_t x, int32_t z)
{
    if (item->pos.y_rot == 0) {
        return (WALL_L - x) & (WALL_L - 1);
    } else if (item->pos.y_rot == -PHD_180) {
        return x & (WALL_L - 1);
    } else if (item->pos.y_rot == PHD_90) {
        return z & (WALL_L - 1);
    } else {
        return (WALL_L - z) & (WALL_L - 1);
    }
}

void BridgeTilt1Floor(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height)
{
    int32_t level = item->pos.y + (GetOffset(item, x, z) >> 2);

    // if (g_Config.fix_grab) {
    //     if (!isSameSector(item, x, y, z)) {
    //         LOG_DEBUG("BridgeTilt1Floor !isSameSector");
    //         return;
    //     }

    //     if (y > item->pos.y) {
    //         LOG_DEBUG("BridgeTilt1Floor y > item->pos.y");
    //         return;
    //     }

    //     LOG_DEBUG("BridgeTilt1Floor level: %d", level);
    //     *height = level;
    // } else {
    //     if (y <= level) {
    //         *height = level;
    //     }
    // }
    if (y <= level) {
        *height = level;
    }
}

void BridgeTilt1Ceiling(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height)
{
    // if (g_Config.fix_grab) {
    //     if (!isSameSector(item, x, y, z)) {
    //         LOG_DEBUG("BridgeTilt1Ceiling !isSameSector");
    //         return;
    //     }
    // }

    int32_t level = item->pos.y + (GetOffset(item, x, z) >> 2);
    if (y > level) {
        *height = level + STEP_L;
    }
}

void BridgeTilt2Floor(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height)
{
    int32_t level = item->pos.y + (GetOffset(item, x, z) >> 1);

    // if (g_Config.fix_grab) {
    //     if (!isSameSector(item, x, y, z)) {
    //         LOG_DEBUG("BridgeTilt2Floor !isSameSector");
    //         return;
    //     }

    //     if (y > item->pos.y) {
    //         LOG_DEBUG("BridgeTilt2Floor y > item->pos.y");
    //         return;
    //     }

    //     LOG_DEBUG("BridgeTilt2Floor level: %d", level);
    //     *height = level;
    // } else {
    //     if (y <= level) {
    //         *height = level;
    //     }
    // }
    if (y <= level) {
        *height = level;
    }
}

void BridgeTilt2Ceiling(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height)
{
    // if (g_Config.fix_grab) {
    //     if (!isSameSector(item, x, y, z)) {
    //         LOG_DEBUG("BridgeTilt2Ceiling !isSameSector");
    //         return;
    //     }
    // }

    int32_t level = item->pos.y + (GetOffset(item, x, z) >> 1);
    if (y > level) {
        *height = level + STEP_L;
    }
}
