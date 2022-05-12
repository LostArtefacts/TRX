#include "game/creature.h"

#include "game/random.h"
#include "global/vars.h"

void Creature_Initialise(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];

    item->pos.y_rot += (PHD_ANGLE)((Random_GetControl() - PHD_90) >> 1);
    item->collidable = 1;
    item->data = NULL;
}
