#include "game/lara/misc.h"

#include "game/effects.h"
#include "game/items.h"
#include "game/lara/common.h"
#include "game/lara/const.h"
#include "game/random.h"
#include "game/rooms.h"

void Lara_TouchLava(void)
{
    ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    if (lara_item->hit_points < 0 || lara_info->water_status == LWS_CHEAT) {
        return;
    }

    int16_t room_num = lara_item->room_num;
    const SECTOR *const sector = Room_GetSector(
        lara_item->pos.x, MAX_HEIGHT, lara_item->pos.z, &room_num);
    const int32_t height =
        Room_GetHeight(sector, lara_item->pos.x, MAX_HEIGHT, lara_item->pos.z);
    if (lara_item->floor != height) {
        return;
    }

    lara_item->hit_points = -1;
    lara_item->hit_status = 1;

    if (lara_info->water_status != LWS_ABOVE_WATER) {
        return;
    }

    const OBJECT *const obj = Object_Get(O_FLAME);
    for (int32_t i = 0; i < 10; i++) {
        const int16_t effect_num = Effect_Create(lara_item->room_num);
        if (effect_num != NO_EFFECT) {
            EFFECT *const effect = Effect_Get(effect_num);
            effect->object_id = O_FLAME;
            effect->frame_num = obj->mesh_count * Random_GetControl() / 0x7FFF;
            effect->counter = -1 - 24 * Random_GetControl() / 0x7FFF;
        }
    }
}

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

void Lara_UpdateRoomToHeight(const int32_t height)
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
