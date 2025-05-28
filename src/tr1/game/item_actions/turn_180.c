#include "game/item_actions/turn_180.h"

void ItemAction_Turn180(ITEM *item)
{
    item->rot.y += DEG_180;
    item->rot.x *= -1;
}
