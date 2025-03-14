#include "game/lara/misc.h"

#include "game/lara/const.h"
#include "game/rooms.h"

int16_t Lara_FloorFront(
    const ITEM *const item, const int16_t ang, const int32_t dist)
{
    const int32_t x = item->pos.x + ((dist * Math_Sin(ang)) >> W2V_SHIFT);
    const int32_t y = item->pos.y - LARA_HEIGHT;
    const int32_t z = item->pos.z + ((dist * Math_Cos(ang)) >> W2V_SHIFT);
    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(x, y, z, &room_num);
    int32_t height = Room_GetHeight(sector, x, y, z);
    if (height != NO_HEIGHT) {
        height -= item->pos.y;
    }
    return height;
}
