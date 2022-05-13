#include "game/cinema.h"

#include "game/items.h"
#include "global/types.h"
#include "global/vars.h"

void InitialisePlayer1(int16_t item_num)
{
    Item_AddActive(item_num);

    ITEM_INFO *item = &g_Items[item_num];
    g_Camera.pos.room_number = item->room_number;
    g_Camera.pos.x = item->pos.x;
    g_Camera.pos.y = item->pos.y;
    g_Camera.pos.z = item->pos.z;
    g_Camera.target_angle = 0;
    item->pos.y_rot = 0;
}

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

void InitialiseGenPlayer(int16_t item_num)
{
    Item_AddActive(item_num);
    g_Items[item_num].pos.y_rot = 0;
}
