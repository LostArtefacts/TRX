#include "game/creature.h"
#include "game/random.h"

void Creature_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    item->rot.y += (Random_GetControl() - DEG_90) >> 1;
    item->collidable = 1;
    item->data = nullptr;
}

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
