#include "game/objects/creatures/cutscene_player.h"

#include "game/items.h"
#include "global/vars.h"

void CutscenePlayer1_Setup(OBJECT_INFO *obj)
{
    obj->initialise = CutscenePlayer1_Initialise;
    obj->control = CutscenePlayer_Control;
    obj->hit_points = 1;
}

void CutscenePlayer2_Setup(OBJECT_INFO *obj)
{
    obj->initialise = CutscenePlayer_Initialise;
    obj->control = CutscenePlayer_Control;
    obj->hit_points = 1;
}

void CutscenePlayer3_Setup(OBJECT_INFO *obj)
{
    obj->initialise = CutscenePlayer_Initialise;
    obj->control = CutscenePlayer_Control;
    obj->hit_points = 1;
}

void CutscenePlayer4_Setup(OBJECT_INFO *obj)
{
    obj->initialise = CutscenePlayer_Initialise;
    obj->control = CutscenePlayer4_Control;
    obj->hit_points = 1;
}

void CutscenePlayer_Initialise(int16_t item_num)
{
    Item_AddActive(item_num);
    g_Items[item_num].pos.y_rot = 0;
}

void CutscenePlayer_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    item->pos.y_rot = g_Camera.target_angle;
    item->pos.x = g_Camera.pos.x;
    item->pos.y = g_Camera.pos.y;
    item->pos.z = g_Camera.pos.z;
    Item_Animate(item);
}

void CutscenePlayer1_Initialise(int16_t item_num)
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

void CutscenePlayer4_Control(int16_t item_num)
{
    Item_Animate(&g_Items[item_num]);
}
