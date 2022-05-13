#include "game/cinema.h"

#include "game/items.h"
#include "global/types.h"
#include "global/vars.h"

void ControlCinematicPlayer4(int16_t item_num)
{
    Item_Animate(&g_Items[item_num]);
}
