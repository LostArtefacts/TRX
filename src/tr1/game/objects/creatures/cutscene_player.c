#include "game/camera.h"
#include "game/items.h"
#include "global/vars.h"

static void M_Setup(OBJECT *obj);
static void M_Initialise(int16_t item_num);
static void M_Control(int16_t item_num);

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->hit_points = 1;
}

static void M_Initialise(const int16_t item_num)
{
    Item_AddActive(item_num);

    ITEM *const item = Item_Get(item_num);
    if (item->object_id == O_PLAYER_1) {
        g_Camera.pos.room_num = item->room_num;
        CINE_DATA *const cine_data = Camera_GetCineData();
        cine_data->position.pos = item->pos;
        cine_data->position.rot.y = 0;
    }
    item->rot.y = 0;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (item->object_id != O_PLAYER_4) {
        CINE_DATA *const cine_data = Camera_GetCineData();
        item->rot.y = cine_data->position.rot.y;
        item->pos = cine_data->position.pos;
    }
    Item_Animate(item);
}

REGISTER_OBJECT(O_PLAYER_1, M_Setup)
REGISTER_OBJECT(O_PLAYER_2, M_Setup)
REGISTER_OBJECT(O_PLAYER_3, M_Setup)
REGISTER_OBJECT(O_PLAYER_4, M_Setup)
