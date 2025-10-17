#include "game/los.h"

#include "game/items.h"
#include "game/objects.h"
#include "game/rooms.h"
#include "utils.h"

#include <math.h>

#define M_CLIP_1 8
#define M_CLIP_2 8

static int32_t m_LOSRooms[200] = {};
static int32_t m_LOSNumRooms = 0;

// This routine transforms the world-space LOS segment [start,target] into the
// object's local coordinates (undoing its translation and Y-rotation), then
// performs a slab intersection test against that local AABB. The first
// smashable item hit is returned, or NO_ITEM if none.
//
// (AABB = Axis-Aligned Bounding Box. It's the rectangular box defined by
// bounds->min/max along X,Y,Z in the object's local space (no rotation).)
//
// @param start  World-space ray origin
// @param target World-space ray end
// @param item   Item to check
// @return       Whether the item collides with the segment
static bool M_ItemIntersectSegment(
    const XYZ_32 start, const XYZ_32 target, const ITEM *const item,
    XYZ_32 *const out_hit_pos)
{
    const double dx = target.x - start.x;
    const double dy = target.y - start.y;
    const double dz = target.z - start.z;

    // Translate into object-local space
    const double ox = start.x - item->pos.x;
    const double oy = start.y - item->pos.y;
    const double oz = start.z - item->pos.z;

    // Unrotate by -rot.y around Y axis
    const float c = cosf(-item->rot.y * M_PI / DEG_180);
    const float s = sinf(-item->rot.y * M_PI / DEG_180);
    const double lx = ox * c + oz * s;
    const double ly = oy;
    const double lz = oz * c - ox * s;
    const double ldx = dx * c + dz * s;
    const double ldy = dy;
    const double ldz = dz * c - dx * s;

    // Local AABB extents from item's bounds
    const BOUNDS_16 *const orig_bounds = Item_GetBoundsAccurate(item);
    BOUNDS_16 patched_bounds = *orig_bounds;

    const BOUNDS_16 *const bounds = &patched_bounds;

    // Parametric interval [t0..t1] in Q14 fixed-point
    double t0 = 0.0;
    double t1 = 1.0;

    // X slab
    if (ldx != 0) {
        double t_near = (double)(bounds->min.x - lx) / ldx;
        double t_far = (double)(bounds->max.x - lx) / ldx;
        if (t_near > t_far) {
            SWAP(t_near, t_far);
        }
        if (t_near > t1 || t_far < t0) {
            return false;
        }
        CLAMPL(t0, t_near);
        CLAMPG(t1, t_far);
    } else if (lx < bounds->min.x || lx > bounds->max.x) {
        return false;
    }

    // Y slab
    if (ldy != 0) {
        double t_near = (double)(bounds->min.y - ly) / ldy;
        double t_far = (double)(bounds->max.y - ly) / ldy;
        if (t_near > t_far) {
            SWAP(t_near, t_far);
        }
        if (t_near > t1 || t_far < t0) {
            return false;
        }
        CLAMPL(t0, t_near);
        CLAMPG(t1, t_far);
    } else if (ly < bounds->min.y || ly > bounds->max.y) {
        return false;
    }

    // Z slab
    if (ldz != 0) {
        double t_near = (double)(bounds->min.z - lz) / ldz;
        double t_far = (double)(bounds->max.z - lz) / ldz;
        if (t_near > t_far) {
            SWAP(t_near, t_far);
        }
        if (t_near > t1 || t_far < t0) {
            return false;
        }
        CLAMPL(t0, t_near);
        CLAMPG(t1, t_far);
    } else if (lz < bounds->min.z || lz > bounds->max.z) {
        return false;
    }

    if (out_hit_pos != nullptr) {
        // world-space hit position = start + t0 * (target-start)
        out_hit_pos->x = start.x + t0 * dx;
        out_hit_pos->y = start.y + t0 * dy;
        out_hit_pos->z = start.z + t0 * dz;
    }

    return true;
}

// Smashes all objects along the ray path.
//
// @param start  World-space ray origin
// @param target World-space ray end
// @return       First smashable item's index, or NO_ITEM if none hit
int32_t LOS_CheckSmashable(
    const XYZ_32 start, const XYZ_32 target, XYZ_32 *const out_hit_pos)
{
    int32_t best_dist = INT32_MAX;
    int16_t best_item_num = NO_ITEM;

    for (int16_t item_num = 0; item_num < Item_GetTotalCount(); item_num++) {
        const ITEM *const item = Item_Get(item_num);
        if (item->status == IS_DEACTIVATED) {
            continue;
        }
        if (!Object_IsType(item->object_id, g_SmashableObjects)
            && !Object_IsType(item->object_id, g_ShatterableObjects)) {
            continue;
        }

        XYZ_32 hit_pos;
        if (!M_ItemIntersectSegment(start, target, item, &hit_pos)) {
            continue;
        }

        // Ray segment intersects the object's local AABB
        const int32_t dist = Item_GetDistance(item, &start);
        if (dist < best_dist) {
            best_dist = dist;
            best_item_num = item_num;
            if (out_hit_pos != nullptr) {
                *out_hit_pos = hit_pos;
            }
        }
    }

    return best_item_num;
}

static int32_t M_CheckX(
    const GAME_VECTOR *const start, GAME_VECTOR *const target)
{
    const int32_t dx = target->x - start->x;
    if (dx == 0) {
        return 1;
    }

    const int32_t dy = ((target->y - start->y) << WALL_SHIFT) / dx;
    const int32_t dz = ((target->z - start->z) << WALL_SHIFT) / dx;

    int16_t room_num = start->room_num;
    int16_t last_room_num = start->room_num;

    m_LOSRooms[0] = room_num;
    m_LOSNumRooms = 1;

    if (dx < 0) {
        int32_t x = start->x & (~(WALL_L - 1));
        int32_t y = start->y + ((dy * (x - start->x)) >> WALL_SHIFT);
        int32_t z = start->z + ((dz * (x - start->x)) >> WALL_SHIFT);

        while (x > target->x) {
            {
                const SECTOR *const sector = Room_GetSector(x, y, z, &room_num);
                const int32_t height = Room_GetHeight(sector, x, y, z);
                const int32_t ceiling = Room_GetCeiling(sector, x, y, z);
                if (y > height || y < ceiling) {
                    target->x = x;
                    target->y = y;
                    target->z = z;
                    target->room_num = room_num;
                    return -1;
                }
            }

            if (room_num != last_room_num) {
                last_room_num = room_num;
                m_LOSRooms[m_LOSNumRooms++] = room_num;
            }

            {
                const SECTOR *const sector =
                    Room_GetSector(x - 1, y, z, &room_num);
                const int32_t height = Room_GetHeight(sector, x - 1, y, z);
                const int32_t ceiling = Room_GetCeiling(sector, x - 1, y, z);
                if (y > height || y < ceiling) {
                    target->x = x;
                    target->y = y;
                    target->z = z;
                    target->room_num = last_room_num;
                    return 0;
                }
            }

            x -= WALL_L;
            y -= dy;
            z -= dz;
        }
    } else {
        int32_t x = start->x | (WALL_L - 1);
        int32_t y = start->y + (((x - start->x) * dy) >> WALL_SHIFT);
        int32_t z = start->z + (((x - start->x) * dz) >> WALL_SHIFT);

        while (x < target->x) {
            {
                const SECTOR *const sector = Room_GetSector(x, y, z, &room_num);
                const int32_t height = Room_GetHeight(sector, x, y, z);
                const int32_t ceiling = Room_GetCeiling(sector, x, y, z);
                if (y > height || y < ceiling) {
                    target->z = z;
                    target->y = y;
                    target->x = x;
                    target->room_num = room_num;
                    return -1;
                }
            }

            if (room_num != last_room_num) {
                last_room_num = room_num;
                m_LOSRooms[m_LOSNumRooms++] = room_num;
            }

            {
                const SECTOR *const sector =
                    Room_GetSector(x + 1, y, z, &room_num);
                const int32_t height = Room_GetHeight(sector, x + 1, y, z);
                const int32_t ceiling = Room_GetCeiling(sector, x + 1, y, z);
                if (y > height || y < ceiling) {
                    target->x = x;
                    target->y = y;
                    target->z = z;
                    target->room_num = last_room_num;
                    return 0;
                }
            }

            x += WALL_L;
            y += dy;
            z += dz;
        }
    }

    target->room_num = room_num;
    return 1;
}

static int32_t M_CheckZ(
    const GAME_VECTOR *const start, GAME_VECTOR *const target)
{
    const int32_t dz = target->z - start->z;
    if (dz == 0) {
        return 1;
    }

    const int32_t dx = ((target->x - start->x) << WALL_SHIFT) / dz;
    const int32_t dy = ((target->y - start->y) << WALL_SHIFT) / dz;

    int16_t room_num = start->room_num;
    int16_t last_room_num = start->room_num;

    m_LOSRooms[0] = room_num;
    m_LOSNumRooms = 1;

    if (dz < 0) {
        int32_t z = start->z & (~(WALL_L - 1));
        int32_t x = start->x + ((dx * (z - start->z)) >> WALL_SHIFT);
        int32_t y = start->y + ((dy * (z - start->z)) >> WALL_SHIFT);

        while (z > target->z) {
            {
                const SECTOR *const sector = Room_GetSector(x, y, z, &room_num);
                const int32_t height = Room_GetHeight(sector, x, y, z);
                const int32_t ceiling = Room_GetCeiling(sector, x, y, z);
                if (y > height || y < ceiling) {
                    target->x = x;
                    target->y = y;
                    target->z = z;
                    target->room_num = room_num;
                    return -1;
                }
            }

            if (room_num != last_room_num) {
                last_room_num = room_num;
                m_LOSRooms[m_LOSNumRooms++] = room_num;
            }

            {
                const SECTOR *const sector =
                    Room_GetSector(x, y, z - 1, &room_num);
                const int32_t height = Room_GetHeight(sector, x, y, z - 1);
                const int32_t ceiling = Room_GetCeiling(sector, x, y, z - 1);
                if (y > height || y < ceiling) {
                    target->x = x;
                    target->y = y;
                    target->z = z;
                    target->room_num = last_room_num;
                    return 0;
                }
            }

            z -= WALL_L;
            x -= dx;
            y -= dy;
        }
    } else {
        int32_t z = start->z | (WALL_L - 1);
        int32_t x = start->x + ((dx * (z - start->z)) >> WALL_SHIFT);
        int32_t y = start->y + ((dy * (z - start->z)) >> WALL_SHIFT);

        while (z < target->z) {
            {
                const SECTOR *const sector = Room_GetSector(x, y, z, &room_num);
                const int32_t height = Room_GetHeight(sector, x, y, z);
                const int32_t ceiling = Room_GetCeiling(sector, x, y, z);
                if (y > height || y < ceiling) {
                    target->x = x;
                    target->y = y;
                    target->z = z;
                    target->room_num = room_num;
                    return -1;
                }
            }

            if (room_num != last_room_num) {
                last_room_num = room_num;
                m_LOSRooms[m_LOSNumRooms++] = room_num;
            }

            {
                const SECTOR *const sector =
                    Room_GetSector(x, y, z + 1, &room_num);
                const int32_t height = Room_GetHeight(sector, x, y, z + 1);
                const int32_t ceiling = Room_GetCeiling(sector, x, y, z + 1);
                if (y > height || y < ceiling) {
                    target->x = x;
                    target->y = y;
                    target->z = z;
                    target->room_num = last_room_num;
                    return 0;
                }
            }

            z += WALL_L;
            x += dx;
            y += dy;
        }
    }

    target->room_num = room_num;
    return 1;
}

static int32_t M_ClipTarget(
    const GAME_VECTOR *const start, GAME_VECTOR *const target)
{
    int16_t room_num = target->room_num;
    const SECTOR *sector =
        Room_GetSector(target->x, target->y, target->z, &room_num);

    if (target->y > Room_GetHeight(sector, target->x, target->y, target->z)) {
        const XYZ_32 origin = {
            .x =
                start->x + ((M_CLIP_1 - 1) * (target->x - start->x) / M_CLIP_1),
            .y =
                start->y + ((M_CLIP_1 - 1) * (target->y - start->y) / M_CLIP_1),
            .z =
                start->z + ((M_CLIP_1 - 1) * (target->z - start->z) / M_CLIP_1),
        };
        int32_t dx, dy, dz;
        for (int32_t i = M_CLIP_2 - 1; i > 0; i--) {
            dx = origin.x + (i * (target->x - origin.x) / M_CLIP_2);
            dy = origin.y + (i * (target->y - origin.y) / M_CLIP_2);
            dz = origin.z + (i * (target->z - origin.z) / M_CLIP_2);
            sector = Room_GetSector(dx, dy, dz, &room_num);
            if (dy < Room_GetHeight(sector, dx, dy, dz)) {
                break;
            }
        }

        target->x = dx;
        target->y = dy;
        target->z = dz;
        target->room_num = room_num;
        return 0;
    }

    if (target->y < Room_GetCeiling(sector, target->x, target->y, target->z)) {
        const XYZ_32 origin = {
            .x =
                start->x + ((M_CLIP_1 - 1) * (target->x - start->x) / M_CLIP_1),
            .y =
                start->y + ((M_CLIP_1 - 1) * (target->y - start->y) / M_CLIP_1),
            .z =
                start->z + ((M_CLIP_1 - 1) * (target->z - start->z) / M_CLIP_1),
        };
        int32_t dx, dy, dz;
        for (int32_t i = M_CLIP_2 - 1; i > 0; i--) {
            dx = origin.x + (i * (target->x - origin.x) / M_CLIP_2);
            dy = origin.y + (i * (target->y - origin.y) / M_CLIP_2);
            dz = origin.z + (i * (target->z - origin.z) / M_CLIP_2);

            sector = Room_GetSector(dx, dy, dz, &room_num);
            if (dy > Room_GetCeiling(sector, dx, dy, dz)) {
                break;
            }
        }

        target->x = dx;
        target->y = dy;
        target->z = dz;
        target->room_num = room_num;
        return 0;
    }

    return 1;
}

bool LOS_Check(const GAME_VECTOR *const start, GAME_VECTOR *const target)
{
    const int32_t dx = ABS(target->x - start->x);
    const int32_t dz = ABS(target->z - start->z);

    int32_t los1;
    int32_t los2;
    if (dz > dx) {
        los1 = M_CheckX(start, target);
        los2 = M_CheckZ(start, target);
    } else {
        los1 = M_CheckZ(start, target);
        los2 = M_CheckX(start, target);
    }

    if (!los2) {
        return false;
    }

    if (dx == 0 && dz == 0) {
        target->room_num = start->room_num;
    }

    return M_ClipTarget(start, target) && los1 == 1 && los2 == 1;
}
