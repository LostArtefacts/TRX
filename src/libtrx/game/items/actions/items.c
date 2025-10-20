#include "game/lara.h"

static void M_Turn180(ITEM *const item)
{
    item->rot.x = -item->rot.x;
    item->rot.y += DEG_180;
    if (item == Lara_GetItem()) {
        LARA_INFO *const lara = Lara_GetLaraInfo();
        lara->move_angle += DEG_180;
    }
}

REGISTER_ITEM_ACTION(ITEM_ACTION_TURN_180, M_Turn180)
