#include "game/effects/turn_180.h"

// original name: turn180_effect
void Turn180(ITEM_INFO *item)
{
    item->pos.y_rot += PHD_180;
}
