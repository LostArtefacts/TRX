#include "game/item_actions/turn_180.h"

#include "game/lara.h"

void ItemAction_Turn180(ITEM *item)
{
    item->rot.y += DEG_180;
    item->rot.x *= -1;
    if (item == Lara_GetItem()) {
        LARA_INFO *const lara = Lara_GetLaraInfo();
        lara->move_angle += DEG_180;
    }
}
