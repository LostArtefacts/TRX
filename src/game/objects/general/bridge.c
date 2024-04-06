#include "game/objects/general/bridge.h"

#include "config.h"
#include "game/items.h"
#include "game/objects/general/cog.h"
#include "game/objects/general/door.h"
#include "game/room.h"
#include "global/const.h"
#include "util.h"

#include <stdbool.h>

static bool Bridge_IsSameSector(
    int32_t x, int32_t y, int32_t z, const ITEM_INFO *item);
static bool Bridge_OnDrawBridge(ITEM_INFO *item, int32_t x, int32_t y);
static int32_t Bridge_GetOffset(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z);
static void Bridge_FixEmbeddedPosition(int16_t item_num);

static bool Bridge_IsSameSector(
    int32_t x, int32_t y, int32_t z, const ITEM_INFO *item)
{
    int32_t sector_x = x / WALL_L;
    int32_t sector_z = z / WALL_L;
    int32_t item_sector_x = item->pos.x / WALL_L;
    int32_t item_sector_z = item->pos.z / WALL_L;

    return sector_x == item_sector_x && sector_z == item_sector_z;
}

static bool Bridge_OnDrawBridge(ITEM_INFO *item, int32_t x, int32_t y)
{
    int32_t ix = item->pos.z >> WALL_SHIFT;
    int32_t iy = item->pos.x >> WALL_SHIFT;

    x >>= WALL_SHIFT;
    y >>= WALL_SHIFT;

    if (item->rot.y == 0 && y == iy && (x == ix - 1 || x == ix - 2)) {
        return true;
    }
    if (item->rot.y == -PHD_180 && y == iy && (x == ix + 1 || x == ix + 2)) {
        return true;
    }
    if (item->rot.y == PHD_90 && x == ix && (y == iy - 1 || y == iy - 2)) {
        return true;
    }
    if (item->rot.y == -PHD_90 && x == ix && (y == iy + 1 || y == iy + 2)) {
        return true;
    }

    return false;
}

static int32_t Bridge_GetOffset(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z)
{
    int32_t offset = 0;
    if (item->rot.y == 0) {
        offset = (WALL_L - x) & (WALL_L - 1);
    } else if (item->rot.y == -PHD_180) {
        offset = x & (WALL_L - 1);
    } else if (item->rot.y == PHD_90) {
        offset = z & (WALL_L - 1);
    } else {
        offset = (WALL_L - z) & (WALL_L - 1);
        // Fixes edge case of an invisible wall on the tilt2 bridge floor
        // Offset would get set to 0 on a specific z pos on bottom of slope
        // This fix sets the offset to the max value (1023) when Lara's at the
        // bottom of the slope
        if (g_Config.fix_bridge_collision && offset == 0 && y < item->pos.y) {
            offset = (WALL_L - z - 1) & (WALL_L - 1);
        }
    }
    return offset;
}

static void Bridge_FixEmbeddedPosition(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];

    int32_t x = item->pos.x;
    int32_t y = item->pos.y;
    int32_t z = item->pos.z;
    int16_t room_num = item->room_number;

    const BOUNDS_16 *bounds = Item_GetBoundsAccurate(item);
    int16_t bridge_height = ABS(bounds->max.y) - ABS(bounds->min.y);

    FLOOR_INFO *floor = Room_GetFloor(x, y - bridge_height, z, &room_num);
    int16_t floor_height = Room_GetHeight(floor, x, y, z);

    // Only move the bridge up if it's at floor level and there
    // isn't a room portal below.
    if (item->floor != floor_height || floor->pit_room != NO_ROOM) {
        return;
    }

    item->pos.y = floor_height - bridge_height;
}

void Bridge_SetupFlat(OBJECT_INFO *obj)
{
    obj->initialise = Bridge_Initialise;
    obj->floor = Bridge_FlatFloor;
    obj->ceiling = Bridge_FlatCeiling;
}

void Bridge_SetupTilt1(OBJECT_INFO *obj)
{
    obj->initialise = Bridge_Initialise;
    obj->floor = Bridge_Tilt1Floor;
    obj->ceiling = Bridge_Tilt1Ceiling;
}

void Bridge_SetupTilt2(OBJECT_INFO *obj)
{
    obj->initialise = Bridge_Initialise;
    obj->floor = Bridge_Tilt2Floor;
    obj->ceiling = Bridge_Tilt2Ceiling;
}

void Bridge_SetupDrawBridge(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->ceiling = Bridge_DrawBridgeCeiling;
    obj->collision = Bridge_DrawBridgeCollision;
    obj->control = Cog_Control;
    obj->save_anim = 1;
    obj->save_flags = 1;
    obj->floor = Bridge_DrawBridgeFloor;
}

void Bridge_Initialise(int16_t item_num)
{
    // Some bridges at floor level are embedded into the floor.
    // This checks if bridges are below a room's floor level
    // and moves them up.
    Bridge_FixEmbeddedPosition(item_num);
}

void Bridge_DrawBridgeFloor(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height)
{
    if (item->current_anim_state != DOOR_OPEN) {
        return;
    }
    if (!Bridge_OnDrawBridge(item, z, x)) {
        return;
    }

    if (y <= item->pos.y) {
        *height = item->pos.y;
    }
}

void Bridge_DrawBridgeCeiling(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height)
{
    if (item->current_anim_state != DOOR_OPEN) {
        return;
    }
    if (!Bridge_OnDrawBridge(item, z, x)) {
        return;
    }

    if (y > item->pos.y) {
        *height = item->pos.y + STEP_L;
    }
}

void Bridge_DrawBridgeCollision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &g_Items[item_num];
    if (item->current_anim_state == DOOR_CLOSED) {
        Door_Collision(item_num, lara_item, coll);
    }
}

void Bridge_FlatFloor(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height)
{
    if (g_Config.fix_bridge_collision && !Bridge_IsSameSector(x, y, z, item)) {
        return;
    }
    if (y > item->pos.y) {
        return;
    }

    *height = item->pos.y;
}

void Bridge_FlatCeiling(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height)
{
    if (g_Config.fix_bridge_collision && !Bridge_IsSameSector(x, y, z, item)) {
        return;
    }

    if (y > item->pos.y) {
        *height = item->pos.y + STEP_L;
    }
}

void Bridge_Tilt1Floor(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height)
{
    if (g_Config.fix_bridge_collision && !Bridge_IsSameSector(x, y, z, item)) {
        return;
    }

    int32_t level = item->pos.y + (Bridge_GetOffset(item, x, y, z) >> 2);
    if (y > level) {
        return;
    }

    *height = level;
}

void Bridge_Tilt1Ceiling(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height)
{
    if (g_Config.fix_bridge_collision && !Bridge_IsSameSector(x, y, z, item)) {
        return;
    }

    int32_t level = item->pos.y + (Bridge_GetOffset(item, x, y, z) >> 2);
    if (y > level) {
        *height = level + STEP_L;
    }
}

void Bridge_Tilt2Floor(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height)
{
    if (g_Config.fix_bridge_collision && !Bridge_IsSameSector(x, y, z, item)) {
        return;
    }

    int32_t level = item->pos.y + (Bridge_GetOffset(item, x, y, z) >> 1);
    if (y > level) {
        return;
    }

    *height = level;
}

void Bridge_Tilt2Ceiling(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height)
{
    if (g_Config.fix_bridge_collision && !Bridge_IsSameSector(x, y, z, item)) {
        return;
    }

    int32_t level = item->pos.y + (Bridge_GetOffset(item, x, y, z) >> 1);
    if (y > level) {
        *height = level + STEP_L;
    }
}
