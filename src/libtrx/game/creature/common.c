#include "game/creature.h"

bool Creature_Activate(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (item->status != IS_INVISIBLE) {
        return true;
    }

    if (!LOT_EnableBaddieAI(item_num, false)) {
        return false;
    }

    item->status = IS_ACTIVE;
    return true;
}
