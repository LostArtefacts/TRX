#include "game/objects/creatures/cutscene_player.h"

#include "game/items.h"
#include "global/vars.h"

void CutscenePlayer_Setup(OBJECT *obj)
{
    obj->initialise = CutscenePlayer_Initialise;
    obj->control = CutscenePlayer_Control;
    obj->hit_points = 1;
}

void CutscenePlayer_Initialise(int16_t item_num)
{
    Item_AddActive(item_num);

    ITEM *const item = &g_Items[item_num];
    if (item->object_id == O_PLAYER_1) {
        g_Camera.pos.room_num = item->room_num;
        g_CinePosition.pos.x = item->pos.x;
        g_CinePosition.pos.y = item->pos.y;
        g_CinePosition.pos.z = item->pos.z;
        g_Camera.target_angle = 0;
    }
    item->rot.y = 0;
}

void CutscenePlayer_Control(int16_t item_num)
{
    ITEM *const item = &g_Items[item_num];
    if (item->object_id != O_PLAYER_4) {
        item->rot.y = g_Camera.target_angle;
        item->pos.x = g_CinePosition.pos.x;
        item->pos.y = g_CinePosition.pos.y;
        item->pos.z = g_CinePosition.pos.z;
    }
    Item_Animate(item);
}
