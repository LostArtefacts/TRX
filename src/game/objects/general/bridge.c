#include "game/objects/general/bridge.h"

#include "config.h"
#include "game/items.h"
#include "game/objects/general/cog.h"
#include "game/objects/general/door.h"
#include "game/room.h"
#include "global/const.h"

#include <libtrx/utils.h>

#include <stdbool.h>

static bool Bridge_IsSameSector(int32_t x, int32_t z, const ITEM_INFO *item);
static bool Bridge_OnDrawBridge(ITEM_INFO *item, int32_t x, int32_t z);
static int32_t Bridge_GetOffset(
    const ITEM_INFO *item, int32_t x, int32_t y, int32_t z);
static void Bridge_FixEmbeddedPosition(int16_t item_num);

static bool Bridge_IsSameSector(int32_t x, int32_t z, const ITEM_INFO *item)
{
    int32_t sector_x = x / WALL_L;
    int32_t sector_z = z / WALL_L;
    int32_t item_sector_x = item->pos.x / WALL_L;
    int32_t item_sector_z = item->pos.z / WALL_L;

    return sector_x == item_sector_x && sector_z == item_sector_z;
}

static bool Bridge_OnDrawBridge(ITEM_INFO *item, int32_t x, int32_t z)
{
    int32_t ix = item->pos.x >> WALL_SHIFT;
    int32_t iz = item->pos.z >> WALL_SHIFT;
    x >>= WALL_SHIFT;
    z >>= WALL_SHIFT;

    if (item->rot.y == 0 && x == ix && (z == iz - 1 || z == iz - 2)) {
        return true;
    } else if (
        item->rot.y == -PHD_180 && x == ix && (z == iz + 1 || z == iz + 2)) {
        return true;
    } else if (
        item->rot.y == PHD_90 && z == iz && (x == ix - 1 || x == ix - 2)) {
        return true;
    } else if (
        item->rot.y == -PHD_90 && z == iz && (x == ix + 1 || x == ix + 2)) {
        return true;
    }

    return false;
}

static int32_t Bridge_GetOffset(
    const ITEM_INFO *const item, int32_t x, int32_t y, int32_t z)
{
    // Set the offset to the max value of 1023 if Lara is outside of the
    // bridge x/z position depending on its angle. This makes sure
    // the height is calculated properly for the front collision since
    // the low end of tilted bridges have a lower height.
    int32_t offset = 0;
    if (item->rot.y == 0) {
        if (g_Config.fix_bridge_collision && x <= item->pos.x - WALL_L / 2) {
            offset = WALL_L - 1;
        } else {
            offset = (WALL_L - x) & (WALL_L - 1);
        }
    } else if (item->rot.y == -PHD_180) {
        if (g_Config.fix_bridge_collision && x >= item->pos.x + WALL_L / 2) {
            offset = 0;
        } else {
            offset = x & (WALL_L - 1);
        }
    } else if (item->rot.y == PHD_90) {
        if (g_Config.fix_bridge_collision && z >= item->pos.z + WALL_L / 2) {
            offset = WALL_L - 1;
        } else {
            offset = z & (WALL_L - 1);
        }
    } else {
        if (g_Config.fix_bridge_collision && z <= item->pos.z - WALL_L / 2) {
            offset = 0;
        } else {
            offset = (WALL_L - z) & (WALL_L - 1);
        }
        // Fixes an edge case of an invisible wall on the tilt2 bridge floor.
        // The offset would get set to 0 on a specific z pos at the bottom of a
        // slope. The game would then set an invisible wall because it thought
        // Lara was at the high end of the tilt2 slope which is higher than a
        // step. This fix sets the offset to the max value (1023) when Lara's at
        // the bottom of the slope.
        if (g_Config.fix_bridge_collision && offset == 0 && y < item->pos.y) {
            offset = (WALL_L - 1 - z) & (WALL_L - 1);
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

    const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(item);
    const int16_t bridge_height = ABS(bounds->max.y) - ABS(bounds->min.y);

    const SECTOR_INFO *const sector =
        Room_GetSector(x, y - bridge_height, z, &room_num);
    const int16_t floor_height = Room_GetHeight(sector, x, y, z);

    // Only move the bridge up if it's at floor level and there
    // isn't a room portal below.
    if (item->floor != floor_height || sector->pit_room != NO_ROOM) {
        return;
    }

    item->pos.y = floor_height - bridge_height;
}

void Bridge_SetupFlat(OBJECT_INFO *obj)
{
    obj->initialise = Bridge_Initialise;
    obj->floor_height_routine = Bridge_AlterFlatFloorHeight;
    obj->ceiling_height_routine = Bridge_AlterFlatCeilingHeight;
}

void Bridge_SetupTilt1(OBJECT_INFO *obj)
{
    obj->initialise = Bridge_Initialise;
    obj->floor_height_routine = Bridge_AlterTilt1FloorHeight;
    obj->ceiling_height_routine = Bridge_AlterTilt1CeilingHeight;
}

void Bridge_SetupTilt2(OBJECT_INFO *obj)
{
    obj->initialise = Bridge_Initialise;
    obj->floor_height_routine = Bridge_AlterTilt2FloorHeight;
    obj->ceiling_height_routine = Bridge_AlterTilt2CeilingHeight;
}

void Bridge_SetupDrawBridge(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->ceiling_height_routine = Bridge_DrawBridgeCeiling;
    obj->collision = Bridge_DrawBridgeCollision;
    obj->control = Cog_Control;
    obj->save_anim = 1;
    obj->save_flags = 1;
    obj->floor_height_routine = Bridge_DrawBridgeFloor;
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

    if (!Bridge_OnDrawBridge(item, x, z)) {
        return;
    }

    if (y > item->pos.y) {
        return;
    }

    if (g_Config.fix_bridge_collision && item->pos.y >= *height) {
        return;
    }

    *height = item->pos.y;
}

void Bridge_DrawBridgeCeiling(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height)
{
    if (item->current_anim_state != DOOR_OPEN) {
        return;
    }

    if (!Bridge_OnDrawBridge(item, x, z)) {
        return;
    }

    if (y <= item->pos.y) {
        return;
    }

    if (g_Config.fix_bridge_collision && item->pos.y <= *height) {
        return;
    }

    *height = item->pos.y + STEP_L;
}

void Bridge_DrawBridgeCollision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &g_Items[item_num];
    if (item->current_anim_state == DOOR_CLOSED) {
        Door_Collision(item_num, lara_item, coll);
    }
}

void Bridge_AlterFlatFloorHeight(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height)
{
    if (g_Config.fix_bridge_collision && !Bridge_IsSameSector(x, z, item)) {
        return;
    }

    if (y > item->pos.y) {
        return;
    }

    if (g_Config.fix_bridge_collision && item->pos.y >= *height) {
        return;
    }

    *height = item->pos.y;
}

void Bridge_AlterFlatCeilingHeight(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height)
{
    if (g_Config.fix_bridge_collision && !Bridge_IsSameSector(x, z, item)) {
        return;
    }

    if (y <= item->pos.y) {
        return;
    }

    if (g_Config.fix_bridge_collision && item->pos.y <= *height) {
        return;
    }

    *height = item->pos.y + STEP_L;
}

void Bridge_AlterTilt1FloorHeight(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height)
{
    if (g_Config.fix_bridge_collision && !Bridge_IsSameSector(x, z, item)) {
        return;
    }

    const int32_t offset_height =
        item->pos.y + (Bridge_GetOffset(item, x, y, z) / 4);
    if (y > offset_height || item->pos.y >= *height) {
        return;
    }

    if (g_Config.fix_bridge_collision && item->pos.y >= *height) {
        return;
    }

    *height = offset_height;
}

void Bridge_AlterTilt1CeilingHeight(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height)
{
    if (g_Config.fix_bridge_collision && !Bridge_IsSameSector(x, z, item)) {
        return;
    }

    const int32_t offset_height =
        item->pos.y + (Bridge_GetOffset(item, x, y, z) / 4);
    if (y <= offset_height) {
        return;
    }

    if (g_Config.fix_bridge_collision && item->pos.y <= *height) {
        return;
    }

    *height = offset_height + STEP_L;
}

void Bridge_AlterTilt2FloorHeight(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height)
{
    if (g_Config.fix_bridge_collision && !Bridge_IsSameSector(x, z, item)) {
        return;
    }

    const int32_t offset_height =
        item->pos.y + (Bridge_GetOffset(item, x, y, z) / 2);
    if (y > offset_height) {
        return;
    }

    if (g_Config.fix_bridge_collision && item->pos.y >= *height) {
        return;
    }

    *height = offset_height;
}

void Bridge_AlterTilt2CeilingHeight(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height)
{
    if (g_Config.fix_bridge_collision && !Bridge_IsSameSector(x, z, item)) {
        return;
    }

    const int32_t offset_height =
        item->pos.y + (Bridge_GetOffset(item, x, y, z) / 2);
    if (y <= offset_height) {
        return;
    }

    if (g_Config.fix_bridge_collision && item->pos.y <= *height) {
        return;
    }

    *height = offset_height + STEP_L;
}
