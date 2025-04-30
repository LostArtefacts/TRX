#include "game/lara/misc.h"

#include "game/items.h"
#include "game/lara/common.h"
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

void Lara_GetCollisionInfo(const ITEM *const item, COLL_INFO *const coll)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    coll->facing = lara->move_angle;
    Collide_GetCollisionInfo(
        coll, item->pos.x, item->pos.y, item->pos.z, item->room_num,
        LARA_HEIGHT);
}

void Lara_UpdateRoom(const int32_t height)
{
    ITEM *const lara_item = Lara_GetItem();
    const int32_t x = lara_item->pos.x;
    const int32_t y = height + lara_item->pos.y;
    const int32_t z = lara_item->pos.z;

    int16_t room_num = lara_item->room_num;
    const SECTOR *const sector = Room_GetSector(x, y, z, &room_num);
    lara_item->floor = Room_GetHeight(sector, x, y, z);

    const int16_t item_num = Item_GetIndex(lara_item);
    Item_UpdateRoom(item_num, room_num);
}

void Lara_ShiftCol(COLL_INFO *const coll)
{
    ITEM *const lara_item = Lara_GetItem();
    lara_item->pos.x += coll->shift.x;
    lara_item->pos.y += coll->shift.y;
    lara_item->pos.z += coll->shift.z;
    coll->shift.z = 0;
    coll->shift.y = 0;
    coll->shift.x = 0;
}
