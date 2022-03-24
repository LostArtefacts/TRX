#include "game/effect_routines/turn_180.h"

void FX_Turn180(ITEM_INFO *item)
{
    item->pos.y_rot += PHD_180;
}
