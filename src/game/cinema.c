#include "game/cinema.h"

#include "game/items.h"
#include "global/types.h"
#include "global/vars.h"

void ControlCinematicPlayer(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    item->pos.y_rot = g_Camera.target_angle;
    item->pos.x = g_Camera.pos.x;
    item->pos.y = g_Camera.pos.y;
    item->pos.z = g_Camera.pos.z;
    Item_Animate(item);
}

void ControlCinematicPlayer4(int16_t item_num)
{
    Item_Animate(&g_Items[item_num]);
}
