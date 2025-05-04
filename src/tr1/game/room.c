#include "game/room.h"

void Room_GetNearbyRooms(
    int32_t x, int32_t y, int32_t z, int32_t r, int32_t h, int16_t room_num)
{
    Room_DrawReset();
    Room_MarkToBeDrawn(room_num);
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
    Room_GetSector(x, y, z, &room_num);
    Room_MarkToBeDrawn(room_num);
}
