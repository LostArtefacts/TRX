#include "game/cutscene.h"
#include "game/objects/common.h"

#include <libtrx/game/collision.h>

static void M_Initialise(const int16_t item_num)
{
    Item_AddActive(item_num);
    ITEM *const item = Item_Get(item_num);
    item->rot.y = 0;
    item->dynamic_light = false;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    item->status = IS_ACTIVE;
    CAMERA_INFO *const camera = Cutscene_GetCamera();
    item->rot.y = camera->target_angle;
    item->pos.x = camera->pos.pos.x;
    item->pos.y = camera->pos.pos.y;
    item->pos.z = camera->pos.pos.z;

    XYZ_32 pos = {};
    Collide_GetJointAbsPosition(item, &pos, 0);

    const int16_t room_num = Room_GetIndexFromPos(pos.x, pos.y, pos.z);
    if (room_num != NO_ROOM) {
        Item_UpdateRoom(item_num, room_num);
    }

    if (item->dynamic_light && item->status != IS_INVISIBLE) {
        pos.x = 0;
        pos.y = 0;
        pos.z = 0;
        Collide_GetJointAbsPosition(item, &pos, 0);
        Output_AddDynamicLight(pos, 12, 11);
    }

    Item_Animate(item);
}

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->hit_points = 1;
}

REGISTER_OBJECT(O_PLAYER_1, M_Setup)
REGISTER_OBJECT(O_PLAYER_2, M_Setup)
REGISTER_OBJECT(O_PLAYER_3, M_Setup)
REGISTER_OBJECT(O_PLAYER_4, M_Setup)
REGISTER_OBJECT(O_PLAYER_5, M_Setup)
REGISTER_OBJECT(O_PLAYER_6, M_Setup)
REGISTER_OBJECT(O_PLAYER_7, M_Setup)
REGISTER_OBJECT(O_PLAYER_8, M_Setup)
REGISTER_OBJECT(O_PLAYER_9, M_Setup)
REGISTER_OBJECT(O_PLAYER_10, M_Setup)
