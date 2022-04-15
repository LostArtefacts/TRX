#include "game/control.h"

#include "game/room.h"
#include "game/shell.h"
#include "global/vars.h"

// TODO: some of these functions have side effects, make them go away

int32_t GetChange(ITEM_INFO *item, ANIM_STRUCT *anim)
{
    if (item->current_anim_state == item->goal_anim_state) {
        return 0;
    }

    ANIM_CHANGE_STRUCT *change = &g_AnimChanges[anim->change_index];
    for (int i = 0; i < anim->number_changes; i++, change++) {
        if (change->goal_anim_state == item->goal_anim_state) {
            ANIM_RANGE_STRUCT *range = &g_AnimRanges[change->range_index];
            for (int j = 0; j < change->number_ranges; j++, range++) {
                if (item->frame_number >= range->start_frame
                    && item->frame_number <= range->end_frame) {
                    item->anim_number = range->link_anim_num;
                    item->frame_number = range->link_frame_num;
                    return 1;
                }
            }
        }
    }

    return 0;
}

FLOOR_INFO *GetFloor(int32_t x, int32_t y, int32_t z, int16_t *room_num)
{
    int16_t data;
    FLOOR_INFO *floor;
    ROOM_INFO *r = &g_RoomInfo[*room_num];
    do {
        int32_t x_floor = (z - r->z) >> WALL_SHIFT;
        int32_t y_floor = (x - r->x) >> WALL_SHIFT;

        if (x_floor <= 0) {
            x_floor = 0;
            if (y_floor < 1) {
                y_floor = 1;
            } else if (y_floor > r->y_size - 2) {
                y_floor = r->y_size - 2;
            }
        } else if (x_floor >= r->x_size - 1) {
            x_floor = r->x_size - 1;
            if (y_floor < 1) {
                y_floor = 1;
            } else if (y_floor > r->y_size - 2) {
                y_floor = r->y_size - 2;
            }
        } else if (y_floor < 0) {
            y_floor = 0;
        } else if (y_floor >= r->y_size) {
            y_floor = r->y_size - 1;
        }

        floor = &r->floor[x_floor + y_floor * r->x_size];
        if (!floor->index) {
            break;
        }

        data = GetDoor(floor);
        if (data != NO_ROOM) {
            *room_num = data;
            r = &g_RoomInfo[data];
        }
    } while (data != NO_ROOM);

    if (y >= ((int32_t)floor->floor << 8)) {
        do {
            if (floor->pit_room == NO_ROOM) {
                break;
            }

            *room_num = floor->pit_room;

            r = &g_RoomInfo[floor->pit_room];
            int32_t x_floor = (z - r->z) >> WALL_SHIFT;
            int32_t y_floor = (x - r->x) >> WALL_SHIFT;
            floor = &r->floor[x_floor + y_floor * r->x_size];
        } while (y >= ((int32_t)floor->floor << 8));
    } else if (y < ((int32_t)floor->ceiling << 8)) {
        do {
            if (floor->sky_room == NO_ROOM) {
                break;
            }

            *room_num = floor->sky_room;

            r = &g_RoomInfo[floor->sky_room];
            int32_t x_floor = (z - r->z) >> WALL_SHIFT;
            int32_t y_floor = (x - r->x) >> WALL_SHIFT;
            floor = &r->floor[x_floor + y_floor * r->x_size];
        } while (y < ((int32_t)floor->ceiling << 8));
    }

    return floor;
}

int16_t GetDoor(FLOOR_INFO *floor)
{
    if (!floor->index) {
        return NO_ROOM;
    }

    int16_t *data = &g_FloorData[floor->index];
    int16_t type = *data++;

    if (type == FT_TILT) {
        data++;
        type = *data++;
    }

    if (type == FT_ROOF) {
        data++;
        type = *data++;
    }

    if ((type & DATA_TYPE) == FT_DOOR) {
        return *data;
    }
    return NO_ROOM;
}

int32_t LOS(GAME_VECTOR *start, GAME_VECTOR *target)
{
    int32_t los1;
    int32_t los2;

    if (ABS(target->z - start->z) > ABS(target->x - start->x)) {
        los1 = xLOS(start, target);
        los2 = zLOS(start, target);
    } else {
        los1 = zLOS(start, target);
        los2 = xLOS(start, target);
    }

    if (!los2) {
        return 0;
    }

    FLOOR_INFO *floor =
        GetFloor(target->x, target->y, target->z, &target->room_number);

    if (ClipTarget(start, target, floor) && los1 == 1 && los2 == 1) {
        return 1;
    }

    return 0;
}

int32_t zLOS(GAME_VECTOR *start, GAME_VECTOR *target)
{
    FLOOR_INFO *floor;

    int32_t dz = target->z - start->z;
    if (dz == 0) {
        return 1;
    }

    int32_t dx = ((target->x - start->x) << WALL_SHIFT) / dz;
    int32_t dy = ((target->y - start->y) << WALL_SHIFT) / dz;

    int16_t room_num = start->room_number;
    int16_t last_room;

    if (dz < 0) {
        int32_t z = start->z & ~(WALL_L - 1);
        int32_t x = start->x + ((dx * (z - start->z)) >> WALL_SHIFT);
        int32_t y = start->y + ((dy * (z - start->z)) >> WALL_SHIFT);

        while (z > target->z) {
            floor = GetFloor(x, y, z, &room_num);
            if (y > Room_GetHeight(floor, x, y, z)
                || y < Room_GetCeiling(floor, x, y, z)) {
                target->x = x;
                target->y = y;
                target->z = z;
                target->room_number = room_num;
                return -1;
            }

            last_room = room_num;

            floor = GetFloor(x, y, z - 1, &room_num);
            if (y > Room_GetHeight(floor, x, y, z - 1)
                || y < Room_GetCeiling(floor, x, y, z - 1)) {
                target->x = x;
                target->y = y;
                target->z = z;
                target->room_number = last_room;
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
            floor = GetFloor(x, y, z, &room_num);
            if (y > Room_GetHeight(floor, x, y, z)
                || y < Room_GetCeiling(floor, x, y, z)) {
                target->x = x;
                target->y = y;
                target->z = z;
                target->room_number = room_num;
                return -1;
            }

            last_room = room_num;

            floor = GetFloor(x, y, z + 1, &room_num);
            if (y > Room_GetHeight(floor, x, y, z + 1)
                || y < Room_GetCeiling(floor, x, y, z + 1)) {
                target->x = x;
                target->y = y;
                target->z = z;
                target->room_number = last_room;
                return 0;
            }

            z += WALL_L;
            x += dx;
            y += dy;
        }
    }

    target->room_number = room_num;
    return 1;
}

int32_t xLOS(GAME_VECTOR *start, GAME_VECTOR *target)
{
    FLOOR_INFO *floor;

    int32_t dx = target->x - start->x;
    if (dx == 0) {
        return 1;
    }

    int32_t dy = ((target->y - start->y) << WALL_SHIFT) / dx;
    int32_t dz = ((target->z - start->z) << WALL_SHIFT) / dx;

    int16_t room_num = start->room_number;
    int16_t last_room;

    if (dx < 0) {
        int32_t x = start->x & ~(WALL_L - 1);
        int32_t y = start->y + ((dy * (x - start->x)) >> WALL_SHIFT);
        int32_t z = start->z + ((dz * (x - start->x)) >> WALL_SHIFT);

        while (x > target->x) {
            floor = GetFloor(x, y, z, &room_num);
            if (y > Room_GetHeight(floor, x, y, z)
                || y < Room_GetCeiling(floor, x, y, z)) {
                target->x = x;
                target->y = y;
                target->z = z;
                target->room_number = room_num;
                return -1;
            }

            last_room = room_num;

            floor = GetFloor(x - 1, y, z, &room_num);
            if (y > Room_GetHeight(floor, x - 1, y, z)
                || y < Room_GetCeiling(floor, x - 1, y, z)) {
                target->x = x;
                target->y = y;
                target->z = z;
                target->room_number = last_room;
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
            floor = GetFloor(x, y, z, &room_num);
            if (y > Room_GetHeight(floor, x, y, z)
                || y < Room_GetCeiling(floor, x, y, z)) {
                target->x = x;
                target->y = y;
                target->z = z;
                target->room_number = room_num;
                return -1;
            }

            last_room = room_num;

            floor = GetFloor(x + 1, y, z, &room_num);
            if (y > Room_GetHeight(floor, x + 1, y, z)
                || y < Room_GetCeiling(floor, x + 1, y, z)) {
                target->x = x;
                target->y = y;
                target->z = z;
                target->room_number = last_room;
                return 0;
            }

            x += WALL_L;
            y += dy;
            z += dz;
        }
    }

    target->room_number = room_num;
    return 1;
}

int32_t ClipTarget(GAME_VECTOR *start, GAME_VECTOR *target, FLOOR_INFO *floor)
{
    int32_t dx = target->x - start->x;
    int32_t dy = target->y - start->y;
    int32_t dz = target->z - start->z;

    int32_t height = Room_GetHeight(floor, target->x, target->y, target->z);
    if (target->y > height && start->y < height) {
        target->y = height;
        target->x = start->x + dx * (height - start->y) / dy;
        target->z = start->z + dz * (height - start->y) / dy;
        return 0;
    }

    int32_t ceiling = Room_GetCeiling(floor, target->x, target->y, target->z);
    if (target->y < ceiling && start->y > ceiling) {
        target->y = ceiling;
        target->x = start->x + dx * (ceiling - start->y) / dy;
        target->z = start->z + dz * (ceiling - start->y) / dy;
        return 0;
    }

    return 1;
}
