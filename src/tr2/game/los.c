#include "game/los.h"

#include <libtrx/debug.h>
#include <libtrx/game/const.h>
#include <libtrx/game/math.h>
#include <libtrx/game/objects/vars.h>
#include <libtrx/game/rooms.h>
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
