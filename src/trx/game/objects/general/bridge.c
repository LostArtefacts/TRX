#include <trx/config.h>
#include <trx/game/objects.h>
#include <trx/game/rooms.h>

static bool M_IsSameSector(const XYZ_32 pos, const ITEM *const item)
{
    const int32_t sector_x = pos.x / WALL_L;
    const int32_t sector_z = pos.z / WALL_L;
    const int32_t item_sector_x = item->pos.x / WALL_L;
    const int32_t item_sector_z = item->pos.z / WALL_L;

    return sector_x == item_sector_x && sector_z == item_sector_z;
}

static int32_t M_GetOffset(const ITEM *const item, const XYZ_32 pos)
{
    // Set the offset to the max value of 1023 if Lara is outside of the
    // bridge x/z position depending on its angle. This makes sure
    // the height is calculated properly for the front collision since
    // the low end of tilted bridges have a lower height.
    int32_t offset = 0;
    if (item->rot.y == 0) {
        if (g_Config.gameplay.fix_bridge_collision
            && pos.x <= item->pos.x - WALL_L / 2) {
            offset = WALL_L - 1;
        } else {
            offset = (WALL_L - pos.x) & (WALL_L - 1);
        }
    } else if (item->rot.y == -DEG_180) {
        if (g_Config.gameplay.fix_bridge_collision
            && pos.x >= item->pos.x + WALL_L / 2) {
            offset = 0;
        } else {
            offset = pos.x & (WALL_L - 1);
        }
    } else if (item->rot.y == DEG_90) {
        if (g_Config.gameplay.fix_bridge_collision
            && pos.z >= item->pos.z + WALL_L / 2) {
            offset = WALL_L - 1;
        } else {
            offset = pos.z & (WALL_L - 1);
        }
    } else {
        if (g_Config.gameplay.fix_bridge_collision
            && pos.z <= item->pos.z - WALL_L / 2) {
            offset = 0;
        } else {
            offset = (WALL_L - pos.z) & (WALL_L - 1);
        }
        // Fixes an edge case of an invisible wall on the tilt2 bridge floor.
        // The offset would get set to 0 on a specific z pos at the bottom of a
        // slope. The game would then set an invisible wall because it thought
        // Lara was at the high end of the tilt2 slope which is higher than a
        // step. This fix sets the offset to the max value (1023) when Lara's at
        // the bottom of the slope.
        if (g_Config.gameplay.fix_bridge_collision && offset == 0
            && pos.y < item->pos.y) {
            offset = (WALL_L - 1 - pos.z) & (WALL_L - 1);
        }
    }
    return offset;
}

static void M_FixEmbeddedPosition(const int16_t item_num)
{
    // Some bridges at floor level are embedded into the floor.
    // This checks if bridges are below a room's floor level
    // and moves them up.
    ITEM *const item = Item_Get(item_num);

    const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(item);
    const int16_t bridge_height = ABS(bounds->max.y) - ABS(bounds->min.y);

    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(
        (XYZ_32) { item->pos.x, item->pos.y - bridge_height, item->pos.z },
        &room_num);
    const int32_t floor_height = Room_GetHeight(sector, item->pos);

    // Only move the bridge up if it's at floor level and there
    // isn't a room portal below.
    if (item->pos.y != floor_height || sector->portal_room.pit != NO_ROOM) {
        return;
    }

    item->pos.y = floor_height - bridge_height;
}

static void M_AddWalkable(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    Walkable_Add(item_num, item->pos);
}

static int32_t M_GetOffsetHeight(const ITEM *const item, const XYZ_32 pos)
{
    switch (item->object_id) {
    case O_BRIDGE_TILT_1:
        return item->pos.y + M_GetOffset(item, pos) / 4;
    case O_BRIDGE_TILT_2:
        return item->pos.y + M_GetOffset(item, pos) / 2;
    default:
        return item->pos.y;
    }
}

static int32_t M_GetFloorHeight(
    const ITEM *const item, const XYZ_32 pos, const int32_t height)
{
    if (g_Config.gameplay.fix_bridge_collision && !M_IsSameSector(pos, item)) {
        return height;
    }

    const int32_t offset_height = M_GetOffsetHeight(item, pos);
    if (pos.y > offset_height) {
        return height;
    }

    if (g_Config.gameplay.fix_bridge_collision && item->pos.y >= height) {
        return height;
    }

    return offset_height;
}

static int32_t M_GetCeilingHeight(
    const ITEM *const item, const XYZ_32 pos, const int32_t height)
{
    if (g_Config.gameplay.fix_bridge_collision && !M_IsSameSector(pos, item)) {
        return height;
    }

    const int32_t offset_height = M_GetOffsetHeight(item, pos);
    if (pos.y <= offset_height) {
        return height;
    }

    if (g_Config.gameplay.fix_bridge_collision && item->pos.y <= height) {
        return height;
    }

    return offset_height + STEP_L;
}

static void M_Initialise(const int16_t item_num)
{
    M_FixEmbeddedPosition(item_num);
    Walkable_AllocateNodes(Item_Get(item_num), 1);
}

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->floor_height_func = M_GetFloorHeight;
    obj->ceiling_height_func = M_GetCeilingHeight;
    obj->add_walkable_func = M_AddWalkable;
}

REGISTER_OBJECT(O_BRIDGE_FLAT, M_Setup)
REGISTER_OBJECT(O_BRIDGE_TILT_1, M_Setup)
REGISTER_OBJECT(O_BRIDGE_TILT_2, M_Setup)
