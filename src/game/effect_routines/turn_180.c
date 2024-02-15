#include "game/effect_routines/turn_180.h"

#include "global/const.h"

void FX_Turn180(ITEM_INFO *item)
{
    item->rot.y += PHD_180;
    item->rot.x *= -1;
}
