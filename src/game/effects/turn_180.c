#include "game/effects/turn_180.h"

void Turn180(ITEM_INFO *item)
{
    item->pos.y_rot += PHD_180;
}
