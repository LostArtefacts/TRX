#include "game/room.h"

void Room_MarkToBeDrawn(int16_t room_num);

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

void Room_GetNearbyRooms(
    const int32_t x, const int32_t y, const int32_t z, const int32_t r,
    const int32_t h, const int16_t room_num)
{
    Room_DrawReset();
    Room_MarkToBeDrawn(room_num);

    Room_GetNewRoom(r + x, y, r + z, room_num);
    Room_GetNewRoom(x - r, y, r + z, room_num);
    Room_GetNewRoom(r + x, y, z - r, room_num);
    Room_GetNewRoom(x - r, y, z - r, room_num);
    Room_GetNewRoom(r + x, y - h, r + z, room_num);
    Room_GetNewRoom(x - r, y - h, r + z, room_num);
    Room_GetNewRoom(r + x, y - h, z - r, room_num);
    Room_GetNewRoom(x - r, y - h, z - r, room_num);
}

void Room_GetNewRoom(
    const int32_t x, const int32_t y, const int32_t z, int16_t room_num)
{
    Room_GetSector(x, y, z, &room_num);
    Room_MarkToBeDrawn(room_num);
}

void Room_InitCinematic(void)
{
    const int32_t room_count = Room_GetCount();
    for (int32_t i = 0; i < room_count; i++) {
        ROOM *const room = Room_Get(i);
        if (room->flipped_room != NO_ROOM_NEG) {
            Room_Get(room->flipped_room)->bound_active = 1;
        }
        room->flags |= RF_OUTSIDE;
    }

    Room_DrawReset();
    for (int32_t i = 0; i < room_count; i++) {
        if (!Room_Get(i)->bound_active) {
            Room_MarkToBeDrawn(i);
        }
    }
}
