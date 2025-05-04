#include "game/rooms/geometry.h"

#include "debug.h"
#include "game/camera.h"
#include "game/objects.h"
#include "game/rooms.h"
#include "utils.h"

#define M_NEG_TILT(T, H) ((T * (H & (WALL_L - 1))) >> 2)
#define M_POS_TILT(T, H) ((T * ((WALL_L - 1 - H) & (WALL_L - 1))) >> 2)

static int16_t m_AbyssMinHeight = 0;
static int32_t m_AbyssMaxHeight = 0;
static HEIGHT_TYPE m_HeightType = HT_WALL;

static int16_t M_GetFloorTiltHeight(
    const SECTOR *sector, int32_t x, int32_t z, bool fix_tilts);
static int16_t M_GetCeilingTiltHeight(
    const SECTOR *sector, int32_t x, int32_t z, bool fix_tilts);

static int16_t M_GetFloorTiltHeight(
    const SECTOR *const sector, const int32_t x, const int32_t z,
    const bool fix_tilts)
{
    int16_t height = sector->floor.height;
    if (sector->floor.tilt == 0 || (height == NO_HEIGHT && fix_tilts)) {
        return height;
    }

    const int32_t z_off = sector->floor.tilt >> 8;
    const int32_t x_off = (int8_t)sector->floor.tilt;

    const HEIGHT_TYPE slope_type =
        (ABS(z_off) > 2 || ABS(x_off) > 2) ? HT_BIG_SLOPE : HT_SMALL_SLOPE;
    if (Camera_IsChunky() && slope_type == HT_BIG_SLOPE) {
        return height;
    }

    m_HeightType = slope_type;

    if (z_off < 0) {
        height -= (int16_t)M_NEG_TILT(z_off, z);
    } else {
        height += (int16_t)M_POS_TILT(z_off, z);
    }

    if (x_off < 0) {
        height -= (int16_t)M_NEG_TILT(x_off, x);
    } else {
        height += (int16_t)M_POS_TILT(x_off, x);
    }

    return height;
}

static int16_t M_GetCeilingTiltHeight(
    const SECTOR *sector, const int32_t x, const int32_t z,
    const bool fix_tilts)
{
    int16_t height = sector->ceiling.height;
    if (sector->ceiling.tilt == 0 || (height == NO_HEIGHT && fix_tilts)) {
        return height;
    }

    const int32_t z_off = sector->ceiling.tilt >> 8;
    const int32_t x_off = (int8_t)sector->ceiling.tilt;

    if (Camera_IsChunky() && (ABS(z_off) > 2 || ABS(x_off) > 2)) {
        return height;
    }

    if (z_off < 0) {
        height += (int16_t)M_NEG_TILT(z_off, z);
    } else {
        height -= (int16_t)M_POS_TILT(z_off, z);
    }

    if (x_off < 0) {
        height += (int16_t)M_POS_TILT(x_off, x);
    } else {
        height -= (int16_t)M_NEG_TILT(x_off, x);
    }

    return height;
}

SECTOR *Room_GetSector(
    const int32_t x, const int32_t y, const int32_t z, int16_t *const room_num)
{
    SECTOR *sector = nullptr;

    while (true) {
        const ROOM *room = Room_Get(*room_num);
        int32_t z_sector = (z - room->pos.z) >> WALL_SHIFT;
        int32_t x_sector = (x - room->pos.x) >> WALL_SHIFT;

        if (z_sector <= 0) {
            z_sector = 0;
            if (x_sector < 1) {
                x_sector = 1;
            } else if (x_sector > room->size.x - 2) {
                x_sector = room->size.x - 2;
            }
        } else if (z_sector >= room->size.z - 1) {
            z_sector = room->size.z - 1;
            if (x_sector < 1) {
                x_sector = 1;
            } else if (x_sector > room->size.x - 2) {
                x_sector = room->size.x - 2;
            }
        } else if (x_sector < 0) {
            x_sector = 0;
        } else if (x_sector >= room->size.x) {
            x_sector = room->size.x - 1;
        }

        sector = Room_GetUnitSector(room, x_sector, z_sector);
        if (sector->portal_room.wall == NO_ROOM) {
            break;
        }
        *room_num = sector->portal_room.wall;
    }

    ASSERT(sector != nullptr);

    if (y >= sector->floor.height) {
        while (sector->portal_room.pit != NO_ROOM) {
            *room_num = sector->portal_room.pit;
            const ROOM *const room = Room_Get(*room_num);
            sector = Room_GetWorldSector(room, x, z);
            if (y < sector->floor.height) {
                break;
            }
        }
    } else if (y < sector->ceiling.height) {
        while (sector->portal_room.sky != NO_ROOM) {
            *room_num = sector->portal_room.sky;
            const ROOM *const room = Room_Get(sector->portal_room.sky);
            sector = Room_GetWorldSector(room, x, z);
            if (y >= sector->ceiling.height) {
                break;
            }
        }
    }

    return sector;
}

SECTOR *Room_GetWorldSector(
    const ROOM *const room, const int32_t x_pos, const int32_t z_pos)
{
    int32_t x_sector = (x_pos - room->pos.x) >> WALL_SHIFT;
    int32_t z_sector = (z_pos - room->pos.z) >> WALL_SHIFT;
    CLAMP(x_sector, 0, room->size.x - 1);
    CLAMP(z_sector, 0, room->size.z - 1);
    return Room_GetUnitSector(room, x_sector, z_sector);
}

SECTOR *Room_GetUnitSector(
    const ROOM *const room, const int32_t x_sector, const int32_t z_sector)
{
    return &room->sectors[z_sector + x_sector * room->size.z];
}

SECTOR *Room_GetPitSector(
    const SECTOR *sector, const int32_t x, const int32_t z)
{
    while (sector->portal_room.pit != NO_ROOM) {
        const ROOM *const room = Room_Get(sector->portal_room.pit);
        sector = Room_GetWorldSector(room, x, z);
    }

    return (SECTOR *)sector;
}

SECTOR *Room_GetSkySector(
    const SECTOR *sector, const int32_t x, const int32_t z)
{
    while (sector->portal_room.sky != NO_ROOM) {
        const ROOM *const room = Room_Get(sector->portal_room.sky);
        sector = Room_GetWorldSector(room, x, z);
    }

    return (SECTOR *)sector;
}

void Room_SetAbyssHeight(const int16_t height)
{
    // Once Lara reaches the min abyss height, she will be killed; she will
    // continue to fall however, so the max height is needed until the inventory
    // is shown, otherwise Lara will hit the floor.
    m_AbyssMinHeight = height;
    m_AbyssMaxHeight = height == 0 ? 0 : m_AbyssMinHeight + 26 * STEP_L;
    CLAMPG(m_AbyssMaxHeight, MAX_HEIGHT - STEP_L);
}

bool Room_IsAbyssHeight(const int32_t height)
{
    return m_AbyssMinHeight != 0 && height >= m_AbyssMinHeight;
}

HEIGHT_TYPE Room_GetHeightType(void)
{
    return m_HeightType;
}

int16_t Room_GetHeight(
    const SECTOR *const sector, const int32_t x, const int32_t y,
    const int32_t z)
{
    return Room_GetHeightEx(sector, x, y, z, false);
}

int16_t Room_GetHeightEx(
    const SECTOR *sector, const int32_t x, const int32_t y, const int32_t z,
    const bool fix_tilts)
{
    m_HeightType = HT_WALL;

    const SECTOR *const pit_sector = Room_GetPitSector(sector, x, z);
    int32_t height = pit_sector->floor.height;

    if (Room_IsAbyssHeight(height)) {
        height = m_AbyssMaxHeight;
    } else {
        height = M_GetFloorTiltHeight(pit_sector, x, z, fix_tilts);
    }

    if (pit_sector->trigger == nullptr) {
        return height;
    }

    const TRIGGER_CMD *cmd = pit_sector->trigger->command;
    for (; cmd != nullptr; cmd = cmd->next_cmd) {
        if (cmd->type != TO_OBJECT) {
            continue;
        }

        const ITEM *const item = Item_Get((int16_t)(intptr_t)cmd->parameter);
        const OBJECT *const obj = Object_Get(item->object_id);
        if (obj->floor_height_func != nullptr) {
            height = obj->floor_height_func(item, x, y, z, height);
        }
    }

    return height;
}

int16_t Room_GetCeiling(
    const SECTOR *const sector, const int32_t x, const int32_t y,
    const int32_t z)
{
    return Room_GetCeilingEx(sector, x, y, z, false);
}

int16_t Room_GetCeilingEx(
    const SECTOR *const sector, const int32_t x, const int32_t y,
    const int32_t z, const bool fix_tilts)
{
    const SECTOR *const sky_sector = Room_GetSkySector(sector, x, z);
    int16_t height = M_GetCeilingTiltHeight(sky_sector, x, z, fix_tilts);

    const SECTOR *const pit_sector = Room_GetPitSector(sector, x, z);
    if (pit_sector->trigger == nullptr) {
        return height;
    }

    const TRIGGER_CMD *cmd = pit_sector->trigger->command;
    for (; cmd != nullptr; cmd = cmd->next_cmd) {
        if (cmd->type != TO_OBJECT) {
            continue;
        }

        const ITEM *const item = Item_Get((int16_t)(intptr_t)cmd->parameter);
        const OBJECT *const obj = Object_Get(item->object_id);
        if (obj->ceiling_height_func != nullptr) {
            height = obj->ceiling_height_func(item, x, y, z, height);
        }
    }

    return height;
}

bool Room_IsOnWalkable(
    const SECTOR *sector, const int32_t x, const int32_t y, const int32_t z,
    const int32_t room_height)
{
    sector = Room_GetPitSector(sector, x, z);
    if (sector->trigger == nullptr) {
        return false;
    }

    int16_t height = sector->floor.height;
    bool object_found = false;
    const TRIGGER_CMD *cmd = sector->trigger->command;
    for (; cmd != nullptr; cmd = cmd->next_cmd) {
        if (cmd->type != TO_OBJECT) {
            continue;
        }

        const int16_t item_num = (int16_t)(intptr_t)cmd->parameter;
        const ITEM *const item = Item_Get(item_num);
        const OBJECT *const obj = Object_Get(item->object_id);
        if (obj->floor_height_func != nullptr) {
            height = obj->floor_height_func(item, x, y, z, height);
            object_found = true;
        }
    }

    return object_found && room_height == height;
}
