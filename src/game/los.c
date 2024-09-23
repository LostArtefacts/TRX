#include "game/los.h"

#include "game/room.h"
#include "global/const.h"

#include <libtrx/utils.h>

#include <stdint.h>

static int32_t M_CheckX(const GAME_VECTOR *start, GAME_VECTOR *target);
static int32_t M_CheckZ(const GAME_VECTOR *start, GAME_VECTOR *target);
static bool M_ClipTarget(
    const GAME_VECTOR *start, GAME_VECTOR *target, const SECTOR *sector);

static int32_t M_CheckX(
    const GAME_VECTOR *const start, GAME_VECTOR *const target)
{
    const SECTOR *sector;

    int32_t dx = target->x - start->x;
    if (dx == 0) {
        return 1;
    }

    int32_t dy = ((target->y - start->y) << WALL_SHIFT) / dx;
    int32_t dz = ((target->z - start->z) << WALL_SHIFT) / dx;

    int16_t room_num = start->room_num;
    int16_t last_room;

    if (dx < 0) {
        int32_t x = start->x & ~(WALL_L - 1);
        int32_t y = start->y + ((dy * (x - start->x)) >> WALL_SHIFT);
        int32_t z = start->z + ((dz * (x - start->x)) >> WALL_SHIFT);

        while (x > target->x) {
            sector = Room_GetSector(x, y, z, &room_num);
            if (y > Room_GetHeight(sector, x, y, z)
                || y < Room_GetCeiling(sector, x, y, z)) {
                target->x = x;
                target->y = y;
                target->z = z;
                target->room_num = room_num;
                return -1;
            }

            last_room = room_num;

            sector = Room_GetSector(x - 1, y, z, &room_num);
            if (y > Room_GetHeight(sector, x - 1, y, z)
                || y < Room_GetCeiling(sector, x - 1, y, z)) {
                target->x = x;
                target->y = y;
                target->z = z;
                target->room_num = last_room;
                return 0;
            }

            x -= WALL_L;
            y -= dy;
            z -= dz;
        }
    } else {
        int32_t x = start->x | (WALL_L - 1);
        int32_t y = start->y + ((dy * (x - start->x)) >> WALL_SHIFT);
        int32_t z = start->z + ((dz * (x - start->x)) >> WALL_SHIFT);

        while (x < target->x) {
            sector = Room_GetSector(x, y, z, &room_num);
            if (y > Room_GetHeight(sector, x, y, z)
                || y < Room_GetCeiling(sector, x, y, z)) {
                target->x = x;
                target->y = y;
                target->z = z;
                target->room_num = room_num;
                return -1;
            }

            last_room = room_num;

            sector = Room_GetSector(x + 1, y, z, &room_num);
            if (y > Room_GetHeight(sector, x + 1, y, z)
                || y < Room_GetCeiling(sector, x + 1, y, z)) {
                target->x = x;
                target->y = y;
                target->z = z;
                target->room_num = last_room;
                return 0;
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
    const SECTOR *sector;

    int32_t dz = target->z - start->z;
    if (dz == 0) {
        return 1;
    }

    int32_t dx = ((target->x - start->x) << WALL_SHIFT) / dz;
    int32_t dy = ((target->y - start->y) << WALL_SHIFT) / dz;

    int16_t room_num = start->room_num;
    int16_t last_room;

    if (dz < 0) {
        int32_t z = start->z & ~(WALL_L - 1);
        int32_t x = start->x + ((dx * (z - start->z)) >> WALL_SHIFT);
        int32_t y = start->y + ((dy * (z - start->z)) >> WALL_SHIFT);

        while (z > target->z) {
            sector = Room_GetSector(x, y, z, &room_num);
            if (y > Room_GetHeight(sector, x, y, z)
                || y < Room_GetCeiling(sector, x, y, z)) {
                target->x = x;
                target->y = y;
                target->z = z;
                target->room_num = room_num;
                return -1;
            }

            last_room = room_num;

            sector = Room_GetSector(x, y, z - 1, &room_num);
            if (y > Room_GetHeight(sector, x, y, z - 1)
                || y < Room_GetCeiling(sector, x, y, z - 1)) {
                target->x = x;
                target->y = y;
                target->z = z;
                target->room_num = last_room;
                return 0;
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
            sector = Room_GetSector(x, y, z, &room_num);
            if (y > Room_GetHeight(sector, x, y, z)
                || y < Room_GetCeiling(sector, x, y, z)) {
                target->x = x;
                target->y = y;
                target->z = z;
                target->room_num = room_num;
                return -1;
            }

            last_room = room_num;

            sector = Room_GetSector(x, y, z + 1, &room_num);
            if (y > Room_GetHeight(sector, x, y, z + 1)
                || y < Room_GetCeiling(sector, x, y, z + 1)) {
                target->x = x;
                target->y = y;
                target->z = z;
                target->room_num = last_room;
                return 0;
            }

            z += WALL_L;
            x += dx;
            y += dy;
        }
    }

    target->room_num = room_num;
    return 1;
}

static bool M_ClipTarget(
    const GAME_VECTOR *const start, GAME_VECTOR *const target,
    const SECTOR *const sector)
{
    int32_t dx = target->x - start->x;
    int32_t dy = target->y - start->y;
    int32_t dz = target->z - start->z;

    const int32_t height =
        Room_GetHeight(sector, target->x, target->y, target->z);
    if (target->y > height && start->y < height) {
        target->y = height;
        target->x = start->x + dx * (height - start->y) / dy;
        target->z = start->z + dz * (height - start->y) / dy;
        return false;
    }

    const int32_t ceiling =
        Room_GetCeiling(sector, target->x, target->y, target->z);
    if (target->y < ceiling && start->y > ceiling) {
        target->y = ceiling;
        target->x = start->x + dx * (ceiling - start->y) / dy;
        target->z = start->z + dz * (ceiling - start->y) / dy;
        return false;
    }

    return true;
}

bool LOS_Check(const GAME_VECTOR *const start, GAME_VECTOR *const target)
{
    int32_t los1;
    int32_t los2;

    if (ABS(target->z - start->z) > ABS(target->x - start->x)) {
        los1 = M_CheckX(start, target);
        los2 = M_CheckZ(start, target);
    } else {
        los1 = M_CheckZ(start, target);
        los2 = M_CheckX(start, target);
    }

    if (!los2) {
        return false;
    }

    const SECTOR *const sector =
        Room_GetSector(target->x, target->y, target->z, &target->room_num);

    return M_ClipTarget(start, target, sector) && los1 == 1 && los2 == 1;
}
