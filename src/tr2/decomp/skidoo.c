#include "decomp/skidoo.h"

#include "game/input.h"
#include "game/items.h"
#include "game/room.h"
#include "global/funcs.h"
#include "global/vars.h"

typedef enum {
    SKIDOO_GET_ON_NONE = 0,
    SKIDOO_GET_ON_LEFT = 1,
    SKIDOO_GET_ON_RIGHT = 2,
} SKIDOO_GET_ON_SIDE;

int32_t __cdecl Skidoo_CheckGetOn(const int16_t item_num, COLL_INFO *const coll)
{
    if (!(g_Input & IN_ACTION) || g_Lara.gun_status != LGS_ARMLESS
        || g_LaraItem->gravity) {
        return SKIDOO_GET_ON_NONE;
    }

    ITEM *const item = &g_Items[item_num];
    const int16_t angle = item->rot.y - g_LaraItem->rot.y;

    SKIDOO_GET_ON_SIDE get_on = SKIDOO_GET_ON_NONE;
    if (angle > PHD_45 && angle < PHD_135) {
        get_on = SKIDOO_GET_ON_LEFT;
    } else if (angle > -PHD_135 && angle < -PHD_45) {
        get_on = SKIDOO_GET_ON_RIGHT;
    }

    if (!Item_TestBoundsCollide(item, g_LaraItem, coll->radius)) {
        return SKIDOO_GET_ON_NONE;
    }

    if (!Collide_TestCollision(item, g_LaraItem)) {
        return SKIDOO_GET_ON_NONE;
    }

    int16_t room_num = item->room_num;
    const SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    const int32_t height =
        Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);
    if (height < -32000) {
        return SKIDOO_GET_ON_NONE;
    }

    return get_on;
}
