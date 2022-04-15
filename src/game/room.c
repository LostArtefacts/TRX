#include "game/room.h"

#include "game/control.h"
#include "global/vars.h"

int16_t Room_GetTiltType(FLOOR_INFO *floor, int32_t x, int32_t y, int32_t z)
{
    ROOM_INFO *r;

    while (floor->pit_room != NO_ROOM) {
        r = &g_RoomInfo[floor->pit_room];
        floor = &r->floor
                     [((z - r->z) >> WALL_SHIFT)
                      + ((x - r->x) >> WALL_SHIFT) * r->x_size];
    }

    if (y + 512 < ((int32_t)floor->floor << 8)) {
        return 0;
    }

    if (floor->index) {
        int16_t *data = &g_FloorData[floor->index];
        if ((data[0] & DATA_TYPE) == FT_TILT) {
            return data[1];
        }
    }

    return 0;
}

int32_t Room_FindGridShift(int32_t src, int32_t dst)
{
    int32_t srcw = src >> WALL_SHIFT;
    int32_t dstw = dst >> WALL_SHIFT;
    if (srcw == dstw) {
        return 0;
    }

    src &= WALL_L - 1;
    if (dstw > srcw) {
        return WALL_L - (src - 1);
    } else {
        return -(src + 1);
    }
}

void Room_GetNearByRooms(
    int32_t x, int32_t y, int32_t z, int32_t r, int32_t h, int16_t room_num)
{
    g_RoomsToDrawCount = 0;
    if (g_RoomsToDrawCount + 1 < MAX_ROOMS_TO_DRAW) {
        g_RoomsToDraw[g_RoomsToDrawCount++] = room_num;
    }
    Room_GetNewRoom(x + r, y, z + r, room_num);
    Room_GetNewRoom(x - r, y, z + r, room_num);
    Room_GetNewRoom(x + r, y, z - r, room_num);
    Room_GetNewRoom(x - r, y, z - r, room_num);
    Room_GetNewRoom(x + r, y - h, z + r, room_num);
    Room_GetNewRoom(x - r, y - h, z + r, room_num);
    Room_GetNewRoom(x + r, y - h, z - r, room_num);
    Room_GetNewRoom(x - r, y - h, z - r, room_num);
}

void Room_GetNewRoom(int32_t x, int32_t y, int32_t z, int16_t room_num)
{
    GetFloor(x, y, z, &room_num);

    for (int i = 0; i < g_RoomsToDrawCount; i++) {
        int16_t drawn_room = g_RoomsToDraw[i];
        if (drawn_room == room_num) {
            return;
        }
    }

    if (g_RoomsToDrawCount + 1 < MAX_ROOMS_TO_DRAW) {
        g_RoomsToDraw[g_RoomsToDrawCount++] = room_num;
    }
}

int16_t Room_GetWaterHeight(int32_t x, int32_t y, int32_t z, int16_t room_num)
{
    ROOM_INFO *r = &g_RoomInfo[room_num];

    int16_t data;
    FLOOR_INFO *floor;
    int32_t x_floor, y_floor;

    do {
        x_floor = (z - r->z) >> WALL_SHIFT;
        y_floor = (x - r->x) >> WALL_SHIFT;

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
        data = GetDoor(floor);
        if (data != NO_ROOM) {
            r = &g_RoomInfo[data];
        }
    } while (data != NO_ROOM);

    if (r->flags & RF_UNDERWATER) {
        while (floor->sky_room != NO_ROOM) {
            r = &g_RoomInfo[floor->sky_room];
            if (!(r->flags & RF_UNDERWATER)) {
                break;
            }
            x_floor = (z - r->z) >> WALL_SHIFT;
            y_floor = (x - r->x) >> WALL_SHIFT;
            floor = &r->floor[x_floor + y_floor * r->x_size];
        }
        return floor->ceiling << 8;
    } else {
        while (floor->pit_room != NO_ROOM) {
            r = &g_RoomInfo[floor->pit_room];
            if (r->flags & RF_UNDERWATER) {
                return floor->floor << 8;
            }
            x_floor = (z - r->z) >> WALL_SHIFT;
            y_floor = (x - r->x) >> WALL_SHIFT;
            floor = &r->floor[x_floor + y_floor * r->x_size];
        }
        return NO_HEIGHT;
    }
}
