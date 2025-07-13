#include "game/los.h"

#include "game/objects/vars.h"
#include "global/vars.h"

#include <libtrx/debug.h>
#include <libtrx/game/math.h>
#include <libtrx/utils.h>

static int32_t m_LOSRooms[200] = {};
static int32_t m_LOSNumRooms = 0;

int32_t LOS_CheckX(const GAME_VECTOR *const start, GAME_VECTOR *const target)
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

int32_t LOS_CheckZ(const GAME_VECTOR *const start, GAME_VECTOR *const target)
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

int32_t LOS_ClipTarget(
    const GAME_VECTOR *const start, GAME_VECTOR *const target,
    const SECTOR *const sector)
{
    const int32_t dx = target->x - start->x;
    const int32_t dy = target->y - start->y;
    const int32_t dz = target->z - start->z;

    const int32_t height =
        Room_GetHeight(sector, target->x, target->y, target->z);
    if (target->y > height && start->y < height) {
        target->y = height;
        target->x = start->x + dx * (height - start->y) / dy;
        target->z = start->z + dz * (height - start->y) / dy;
        return 0;
    }

    const int32_t ceiling =
        Room_GetCeiling(sector, target->x, target->y, target->z);
    if (target->y < ceiling && start->y > ceiling) {
        target->y = ceiling;
        target->x = start->x + dx * (ceiling - start->y) / dy;
        target->z = start->z + dz * (ceiling - start->y) / dy;
        return 0;
    }

    return 1;
}

bool LOS_Check(const GAME_VECTOR *const start, GAME_VECTOR *const target)
{
    int32_t los1;
    int32_t los2;

    const int32_t dx = ABS(target->x - start->x);
    const int32_t dz = ABS(target->z - start->z);

    if (dz > dx) {
        los1 = LOS_CheckX(start, target);
        los2 = LOS_CheckZ(start, target);
    } else {
        los1 = LOS_CheckZ(start, target);
        los2 = LOS_CheckX(start, target);
    }

    if (!los2) {
        return false;
    }

    if (dx == 0 && dz == 0) {
        target->room_num = start->room_num;
    }

    const SECTOR *const sector =
        Room_GetSector(target->x, target->y, target->z, &target->room_num);

    if (!LOS_ClipTarget(start, target, sector)) {
        return false;
    }
    if (los1 == 1 && los2 == 1) {
        return true;
    }
    return false;
}

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
// @return       First smashable item's index, or NO_ITEM if none hit
int32_t LOS_CheckSmashable(
    const GAME_VECTOR *const start, const GAME_VECTOR *const target)
{
    const int32_t dx = target->x - start->x;
    const int32_t dy = target->y - start->y;
    const int32_t dz = target->z - start->z;

    for (int32_t i = 0; i < m_LOSNumRooms; i++) {
        for (int16_t item_num = Room_Get(m_LOSRooms[i])->item_num;
             item_num != NO_ITEM; item_num = Item_Get(item_num)->next_item) {
            const ITEM *const item = Item_Get(item_num);
            if (item->status == IS_DEACTIVATED) {
                continue;
            }
            if (!Object_IsType(item->object_id, g_SmashableObjects)) {
                continue;
            }

            // Translate into object-local space
            const int32_t ox = start->x - item->pos.x;
            const int32_t oy = start->y - item->pos.y;
            const int32_t oz = start->z - item->pos.z;
            // Unrotate by -rot.y around Y axis
            const int32_t c = Math_Cos(item->rot.y);
            const int32_t s = Math_Sin(item->rot.y);
            const int32_t lx = ((ox * c) + (oz * s)) >> W2V_SHIFT;
            const int32_t ly = oy;
            const int32_t lz = ((-ox * s) + (oz * c)) >> W2V_SHIFT;
            const int32_t ldx = ((dx * c) + (dz * s)) >> W2V_SHIFT;
            const int32_t ldy = dy;
            const int32_t ldz = (((-dx * s) + (dz * c))) >> W2V_SHIFT;

            // Local AABB extents from item's bounds
            const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(item);

            // Parametric interval [t0..t1] in Q14 fixed-point
            int32_t t0 = 0;
            int32_t t1 = 1 << W2V_SHIFT;

            // X slab
            if (ldx != 0) {
                int32_t tmp;
                int32_t t_near = ((bounds->min.x - lx) << W2V_SHIFT) / ldx;
                int32_t t_far = ((bounds->max.x - lx) << W2V_SHIFT) / ldx;
                if (t_near > t_far) {
                    SWAP(t_near, t_far, tmp);
                }
                if (t_near > t1 || t_far < t0) {
                    continue;
                }
                CLAMPL(t0, t_near);
                CLAMPG(t1, t_far);
            } else if (lx < bounds->min.x || lx > bounds->max.x) {
                continue;
            }

            // Y slab
            if (ldy != 0) {
                int32_t tmp;
                int32_t t_near = ((bounds->min.y - ly) << W2V_SHIFT) / ldy;
                int32_t t_far = ((bounds->max.y - ly) << W2V_SHIFT) / ldy;
                if (t_near > t_far) {
                    SWAP(t_near, t_far, tmp);
                }
                if (t_near > t1 || t_far < t0) {
                    continue;
                }
                CLAMPL(t0, t_near);
                CLAMPG(t1, t_far);
            } else if (ly < bounds->min.y || ly > bounds->max.y) {
                continue;
            }

            // Z slab
            if (ldz != 0) {
                int32_t tmp;
                int32_t t_near = ((bounds->min.z - lz) << W2V_SHIFT) / ldz;
                int32_t t_far = ((bounds->max.z - lz) << W2V_SHIFT) / ldz;
                if (t_near > t_far) {
                    SWAP(t_near, t_far, tmp);
                }
                if (t_near > t1 || t_far < t0) {
                    continue;
                }
                CLAMPL(t0, t_near);
                CLAMPG(t1, t_far);
            } else if (lz < bounds->min.z || lz > bounds->max.z) {
                continue;
            }

            // Ray segment intersects the object's local AABB
            return item_num;
        }
    }
    return NO_ITEM;
}
