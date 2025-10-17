#include "game/los.h"

#include <libtrx/debug.h>
#include <libtrx/game/const.h>
#include <libtrx/game/math.h>
#include <libtrx/game/objects/vars.h>
#include <libtrx/game/rooms.h>
#include <libtrx/utils.h>

#define M_CLIP_1 8
#define M_CLIP_2 8

static int32_t m_LOSRooms[200] = {};
static int32_t m_LOSNumRooms = 0;

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
