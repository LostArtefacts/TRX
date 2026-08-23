#include <trx/game/rooms/geometry.h>

#include <trx/config.h>
#include <trx/core/math/util.h>
#include <trx/core/utils.h>
#include <trx/debug.h>
#include <trx/game/camera.h>
#include <trx/game/items.h>
#include <trx/game/objects.h>
#include <trx/game/rooms.h>

// How many rooms around the mover are searched for something barring the way.
#define M_MAX_BLOCK_ROOMS 20

#define M_WALL_MASK (WALL_L - 1)
#define M_NEG_TILT(T, H) ((T * (H & M_WALL_MASK)) >> 2)
#define M_POS_TILT(T, H) ((T * ((M_WALL_MASK - H) & M_WALL_MASK)) >> 2)

static int32_t m_AbyssMinHeight = 0;
static int32_t m_AbyssMaxHeight = 0;
static HEIGHT_TYPE m_HeightType = HT_WALL;

static inline int32_t M_GetTiltShift(
    const XZ_16 tilt, const int32_t x, const int32_t z, const bool is_ceiling)
{
    int32_t shift = 0;
    if (is_ceiling) {
        if (tilt.z < 0) {
            shift += M_NEG_TILT(tilt.z, z);
        } else {
            shift -= M_POS_TILT(tilt.z, z);
        }

        if (tilt.x < 0) {
            shift += M_POS_TILT(tilt.x, x);
        } else {
            shift -= M_NEG_TILT(tilt.x, x);
        }
    } else {
        if (tilt.z < 0) {
            shift -= M_NEG_TILT(tilt.z, z);
        } else {
            shift += M_POS_TILT(tilt.z, z);
        }

        if (tilt.x < 0) {
            shift -= M_NEG_TILT(tilt.x, x);
        } else {
            shift += M_POS_TILT(tilt.x, x);
        }
    }

    return shift;
}

static int32_t M_GetUnsplitSurfaceHeight(
    const SURFACE surface, const int32_t x, const int32_t z)
{
    int32_t height = surface.height;
    if (surface.tilt.x == 0 && surface.tilt.z == 0) {
        return height;
    }

    const HEIGHT_TYPE slope_type =
        (ABS(surface.tilt.z) > MAX_SLOPE || ABS(surface.tilt.x) > MAX_SLOPE)
        ? HT_BIG_SLOPE
        : HT_SMALL_SLOPE;
    if (Camera_IsChunky() && slope_type == HT_BIG_SLOPE) {
        return height;
    }

    const bool is_ceiling = surface.type == SURFACE_CEILING;
    height += M_GetTiltShift(surface.tilt, x, z, is_ceiling);

    if (!is_ceiling) {
        m_HeightType = slope_type;
    }

    return height;
}

static inline XZ_16 M_GetSplitTilt(
    const SURFACE *const surface, const int32_t x, const int32_t z,
    int32_t *const shift)
{
    const bool is_ceiling = surface->type == SURFACE_CEILING;
    const SPLIT split = surface->split;
    const int32_t dx = x & M_WALL_MASK;
    const int32_t dz = z & M_WALL_MASK;
    const int16_t t0 = split.tilts[0];
    const int16_t t1 = split.tilts[1];
    const int16_t t2 = split.tilts[2];
    const int16_t t3 = split.tilts[3];
    XZ_16 tilt = {};

    if (split.type == SPLIT_NWSE_SOLID || split.type == SPLIT_NWSE_PORTAL_SW
        || split.type == SPLIT_NWSE_PORTAL_NE) {
        if (dx > WALL_L - dz) {
            tilt.x = is_ceiling ? (t0 - t1) : (t3 - t2);
            tilt.z = t3 - t0;
            *shift = split.h1;
        } else {
            tilt.x = is_ceiling ? (t3 - t2) : (t0 - t1);
            tilt.z = t2 - t1;
            *shift = split.h2;
        }
    } else if (dx > dz) {
        tilt.x = is_ceiling ? (t3 - t2) : (t0 - t1);
        tilt.z = t3 - t0;
        *shift = split.h1;
    } else {
        tilt.x = is_ceiling ? (t0 - t1) : (t3 - t2);
        tilt.z = t2 - t1;
        *shift = split.h2;
    }

    return tilt;
}

static int32_t M_GetSplitSurfaceHeight(
    const SURFACE surface, const int32_t x, const int32_t z)
{
    const bool is_ceiling = surface.type == SURFACE_CEILING;
    if (Camera_IsChunky()) {
        const int32_t ch1 = surface.height + surface.split.h2;
        const int32_t ch2 = surface.height + surface.split.h1;
        return is_ceiling ? MAX(ch1, ch2) : MIN(ch1, ch2);
    }

    int32_t height = surface.height;
    if (!is_ceiling) {
        m_HeightType = HT_SPLIT_TRI;
    }

    int32_t shift = 0;
    const XZ_16 tilt = M_GetSplitTilt(&surface, x, z, &shift);
    shift += M_GetTiltShift(tilt, x, z, is_ceiling);
    height += shift;

    if (!is_ceiling) {
        if (ABS(tilt.x) > MAX_SLOPE || ABS(tilt.z) > MAX_SLOPE) {
            m_HeightType = HT_DIAGONAL;
        } else if (m_HeightType != HT_SPLIT_TRI) {
            m_HeightType = HT_SMALL_SLOPE;
        }
    }

    return height;
}

static int32_t M_GetSurfaceHeight(
    const SURFACE surface, const int32_t x, const int32_t z,
    const bool fix_tilts)
{
    if (surface.height == NO_HEIGHT && (surface.is_split || fix_tilts)) {
        return NO_HEIGHT;
    }

    return surface.is_split ? M_GetSplitSurfaceHeight(surface, x, z)
                            : M_GetUnsplitSurfaceHeight(surface, x, z);
}

static bool M_IsPortalSolid(
    const SURFACE surface, const int32_t x, const int32_t z)
{
    if (!surface.is_split) {
        return false;
    }

    const int32_t dx = x & M_WALL_MASK;
    const int32_t dz = z & M_WALL_MASK;
    const bool is_ceiling = surface.type == SURFACE_CEILING;

    switch (surface.split.type) {
    case SPLIT_NWSE_PORTAL_SW:
        return dx > WALL_L - dz;
    case SPLIT_NWSE_PORTAL_NE:
        return dx <= WALL_L - dz;
    case SPLIT_NESW_PORTAL_SE:
        return is_ceiling ? (dx <= dz) : (dx > dz);
    case SPLIT_NESW_PORTAL_NW:
        return is_ceiling ? (dx > dz) : (dx <= dz);
    default:
        return false;
    }
}

bool Room_BoundsReachPortal(
    const BOUNDS_32 *const bounds, const PORTAL *const portal)
{
    if (portal->normal.y == 0) {
        return Bounds32_Intersect(bounds, &portal->bounds);
    }

    if (bounds->min.x > portal->bounds.max.x
        || bounds->max.x < portal->bounds.min.x
        || bounds->min.z > portal->bounds.max.z
        || bounds->max.z < portal->bounds.min.z) {
        return false;
    }

    return portal->normal.y > 0 ? bounds->min.y <= portal->bounds.min.y
                                : bounds->max.y >= portal->bounds.max.y;
}

BOUNDS_32 Room_GetRoomBounds(const ROOM *const room)
{
    ASSERT(room != nullptr);
    return (BOUNDS_32) {
        .min = {
            .x = room->pos.x + WALL_L,
            .y = room->max_ceiling,
            .z = room->pos.z + WALL_L,
        },
        .max = {
            .x = room->pos.x + room->size.x * WALL_L - WALL_L,
            .y = room->min_floor,
            .z = room->pos.z + room->size.z * WALL_L - WALL_L,
        },
    };
}

SECTOR *Room_GetSector(const XYZ_32 pos, int16_t *const room_num)
{
    SECTOR *sector = nullptr;

    while (true) {
        const ROOM *room = Room_Get(*room_num);
        int32_t z_sector = (pos.z - room->pos.z) >> WALL_SHIFT;
        int32_t x_sector = (pos.x - room->pos.x) >> WALL_SHIFT;

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

    if (pos.y >= M_GetSurfaceHeight(sector->floor, pos.x, pos.z, true)) {
        do {
            if (sector->portal_room.pit == NO_ROOM
                || M_IsPortalSolid(sector->floor, pos.x, pos.z)) {
                break;
            }
            *room_num = sector->portal_room.pit;
            const ROOM *const room = Room_Get(*room_num);
            sector = Room_GetWorldSector(room, pos.x, pos.z);
        } while (pos.y
                 >= M_GetSurfaceHeight(sector->floor, pos.x, pos.z, true));
    } else if (
        pos.y < M_GetSurfaceHeight(sector->ceiling, pos.x, pos.z, true)) {
        do {
            if (sector->portal_room.sky == NO_ROOM
                || M_IsPortalSolid(sector->ceiling, pos.x, pos.z)) {
                break;
            }
            *room_num = sector->portal_room.sky;
            const ROOM *const room = Room_Get(sector->portal_room.sky);
            sector = Room_GetWorldSector(room, pos.x, pos.z);
        } while (pos.y
                 < M_GetSurfaceHeight(sector->ceiling, pos.x, pos.z, true));
    }

    return sector;
}

SECTOR *Room_GetSectorOnWalkable(const XYZ_32 pos, int16_t *const room_num)
{
    // Resolve wall portals.
    const ROOM *room = Room_Get(*room_num);
    SECTOR *sector = Room_GetWorldSector(room, pos.x, pos.z);
    while (sector->portal_room.wall != NO_ROOM) {
        *room_num = sector->portal_room.wall;
        room = Room_Get(*room_num);
        sector = Room_GetWorldSector(room, pos.x, pos.z);
    }

    // Check if on a walkable.
    const int32_t room_height = Room_GetHeight(sector, pos);
    const bool skip_pit = Room_IsOnWalkable(
        sector,
        (XYZ_32) {
            pos.x,
            ROUND_TO_HALF_CLICK(pos.y),
            pos.z,
        },
        ROUND_TO_HALF_CLICK(pos.y), NO_ITEM);

    // Traverse pit sector unless on a walkable.
    if (!skip_pit && pos.y >= sector->floor.height) {
        while (sector->portal_room.pit != NO_ROOM) {
            *room_num = sector->portal_room.pit;
            room = Room_Get(*room_num);
            sector = Room_GetWorldSector(room, pos.x, pos.z);
            if (pos.y < sector->floor.height) {
                break;
            }
        }
    } else if (pos.y < sector->ceiling.height) {
        while (sector->portal_room.sky != NO_ROOM) {
            *room_num = sector->portal_room.sky;
            room = Room_Get(*room_num);
            sector = Room_GetWorldSector(room, pos.x, pos.z);
            if (pos.y >= sector->ceiling.height) {
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
    while (sector->portal_room.pit != NO_ROOM
           && !M_IsPortalSolid(sector->floor, x, z)) {
        const ROOM *const room = Room_Get(sector->portal_room.pit);
        sector = Room_GetWorldSector(room, x, z);
    }

    return (SECTOR *)sector;
}

SECTOR *Room_GetSkySector(
    const SECTOR *sector, const int32_t x, const int32_t z)
{
    while (sector->portal_room.sky != NO_ROOM
           && !M_IsPortalSolid(sector->ceiling, x, z)) {
        const ROOM *const room = Room_Get(sector->portal_room.sky);
        sector = Room_GetWorldSector(room, x, z);
    }

    return (SECTOR *)sector;
}

void Room_SetAbyssHeight(const int32_t height)
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

XZ_16 Room_GetTiltType(const SECTOR *sector, const XYZ_32 pos)
{
    sector = Room_GetPitSector(sector, pos.x, pos.z);

    if ((pos.y + STEP_L * 2) < sector->floor.height) {
        return (XZ_16) {};
    }

    if (!sector->floor.is_split) {
        return sector->floor.tilt;
    }

    int32_t shift = 0;
    return M_GetSplitTilt(&sector->floor, pos.x, pos.z, &shift);
}

int32_t Room_GetHeight(const SECTOR *const sector, const XYZ_32 pos)
{
    return Room_GetHeightEx(
        sector, pos, g_Config.gameplay.fix_wall_geometry, NO_ITEM);
}

int32_t Room_GetFloorHeightForSector(
    const SECTOR *const sector, const int32_t x, const int32_t z,
    const bool fix_tilts)
{
    m_HeightType = HT_WALL;
    if (Room_IsAbyssHeight(sector->floor.height)) {
        return m_AbyssMaxHeight;
    }
    return M_GetSurfaceHeight(sector->floor, x, z, fix_tilts);
}

int32_t Room_GetHeightEx(
    const SECTOR *const sector, const XYZ_32 pos, const bool fix_tilts,
    const int16_t ignore_item_num)
{
    m_HeightType = HT_WALL;

    const SECTOR *const pit_sector = Room_GetPitSector(sector, pos.x, pos.z);
    int32_t height = pit_sector->floor.height;

    if (Room_IsAbyssHeight(height)) {
        height = m_AbyssMaxHeight;
    } else {
        height = M_GetSurfaceHeight(pit_sector->floor, pos.x, pos.z, fix_tilts);
    }

    // Climb the stack of walkables. In each iteration the test Y pos is moved
    // up to match the current height, so preventing testing below previous
    // walkables.
    int32_t base_height = height;
    XYZ_32 test_pos = pos;
    for (const WALKABLE *w = pit_sector->walkable; w != nullptr; w = w->next) {
        if (w->item_num == ignore_item_num) {
            continue;
        }
        const ITEM *const item = Item_Get(w->item_num);
        const OBJECT *const obj = Object_Get(item->object_id);
        if (obj->floor_height_func == nullptr) {
            continue;
        }
        height = obj->floor_height_func(item, test_pos, height);
        test_pos.y = MIN(pos.y, height);
    }

    if (base_height != height) {
        // A walkable is present, which always override slopes below.
        m_HeightType = HT_WALL;
    }

    return height;
}

int32_t Room_GetCeiling(const SECTOR *const sector, const XYZ_32 pos)
{
    return Room_GetCeilingEx(sector, pos, g_Config.gameplay.fix_wall_geometry);
}

int32_t Room_GetCeilingEx(
    const SECTOR *const sector, const XYZ_32 pos, const bool fix_tilts)
{
    const SECTOR *const sky_sector = Room_GetSkySector(sector, pos.x, pos.z);
    int32_t height =
        M_GetSurfaceHeight(sky_sector->ceiling, pos.x, pos.z, fix_tilts);

    const SECTOR *const pit_sector = Room_GetPitSector(sector, pos.x, pos.z);

    for (const WALKABLE *w = pit_sector->walkable; w != nullptr; w = w->next) {
        const ITEM *const item = Item_Get(w->item_num);
        const OBJECT *const obj = Object_Get(item->object_id);
        if (obj->ceiling_height_func != nullptr) {
            height = obj->ceiling_height_func(item, pos, height);
        }
    }

    return height;
}

int32_t Room_GetWaterHeight(const XYZ_32 pos, const int16_t room_num)
{
    return Room_GetWaterHeightEx(
        pos, room_num, (ROOM_WATER_HEIGHT_ARGS) { .fix_tilts = true });
}

int32_t Room_GetWaterHeightEx(
    const XYZ_32 pos, int16_t room_num, const ROOM_WATER_HEIGHT_ARGS args)
{
    const int32_t x = pos.x;
    const int32_t y = pos.y;
    const int32_t z = pos.z;
    const SECTOR *sector = nullptr;
    const ROOM *room = nullptr;

    do {
        room = Room_Get(room_num);
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
        room_num = sector->portal_room.wall;
    } while (room_num != NO_ROOM);

    if (room->flags.underwater || room->flags.swamp) {
        while (sector->portal_room.sky != NO_ROOM
               && !M_IsPortalSolid(sector->ceiling, x, z)) {
            room = Room_Get(sector->portal_room.sky);
            if (!room->flags.underwater && !room->flags.swamp) {
                return args.fix_tilts
                    ? M_GetSurfaceHeight(sector->ceiling, x, z, true)
                    : room->min_floor;
            }
            sector = Room_GetWorldSector(room, x, z);
        }

        // Nothing but solid ceiling above the water here.
        if (args.require_air_above) {
            return NO_HEIGHT;
        }
        return args.fix_tilts ? M_GetSurfaceHeight(sector->ceiling, x, z, true)
                              : room->max_ceiling;
    } else {
        while (sector->portal_room.pit != NO_ROOM
               && !M_IsPortalSolid(sector->floor, x, z)) {
            room = Room_Get(sector->portal_room.pit);
            if (room->flags.underwater || room->flags.swamp) {
                return args.fix_tilts
                    ? M_GetSurfaceHeight(sector->floor, x, z, true)
                    : room->max_ceiling;
            }
            sector = Room_GetWorldSector(room, x, z);
        }
        return NO_HEIGHT;
    }
}

int32_t Room_FindGridShift(int32_t src, const int32_t dst)
{
    const int32_t src_w = src >> WALL_SHIFT;
    const int32_t dst_w = dst >> WALL_SHIFT;
    if (src_w == dst_w) {
        return 0;
    }

    src &= WALL_L - 1;
    if (dst_w > src_w) {
        return WALL_L - (src - 1);
    } else {
        return -(src + 1);
    }
}

bool Room_IsPathBlocked(
    const XYZ_32 from, const XYZ_32 to, const int16_t room_num,
    const int32_t height, const int32_t reach)
{
    int16_t rooms[M_MAX_BLOCK_ROOMS];
    const int32_t room_count =
        Room_GetAdjoiningRooms(room_num, rooms, M_MAX_BLOCK_ROOMS);
    for (int32_t i = 0; i < room_count; i++) {
        const ROOM *const room = Room_Get(rooms[i]);
        for (int16_t item_num = room->item_num; item_num != NO_ITEM;
             item_num = Item_Get(item_num)->next_item) {
            const ITEM *const item = Item_Get(item_num);
            const OBJECT *const obj = Object_Get(item->object_id);
            if (obj->block_func != nullptr
                && obj->block_func(item, from, to, height, reach)) {
                return true;
            }
        }
    }
    return false;
}

bool Room_IsOnWalkable(
    const SECTOR *sector, const XYZ_32 pos, const int32_t room_height,
    const int16_t ignore_item_num)
{
    sector = Room_GetPitSector(sector, pos.x, pos.z);

    int32_t height = Room_GetHeight(
        sector, (XYZ_32) { pos.x, sector->floor.height + STEP_L, pos.z });
    bool object_found = false;
    for (const WALKABLE *w = sector->walkable; w != nullptr; w = w->next) {
        // Optionally ignore a walkable.
        if (w->item_num == ignore_item_num) {
            continue;
        }
        const ITEM *const item = Item_Get(w->item_num);
        const OBJECT *const obj = Object_Get(item->object_id);
        if (obj->floor_height_func != nullptr) {
            const int32_t test_height =
                obj->floor_height_func(item, pos, height);
            // If the floor height changed, try to climb the walkable stack.
            if (test_height != height) {
                // Check if height changed, i.e. standing on a walkable.
                height = test_height;
                object_found = true;
            }
        }
    }

    return object_found && room_height == height;
}
