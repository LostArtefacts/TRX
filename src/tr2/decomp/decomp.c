#include "decomp/decomp.h"

#include "game/cutscene.h"
#include "game/level.h"

#include <libtrx/config.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/collision.h>
#include <libtrx/game/game.h>
#include <libtrx/game/game_flow.h>
#include <libtrx/game/game_string_table.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/music.h>
#include <libtrx/game/objects/vars.h>
#include <libtrx/game/output.h>
#include <libtrx/game/viewport.h>
#include <libtrx/utils.h>

void Lara_Control_Cutscene(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    CAMERA_INFO *const camera = Cutscene_GetCamera();
    item->rot.y = camera->target_angle;
    item->pos.x = camera->pos.pos.x;
    item->pos.y = camera->pos.pos.y;
    item->pos.z = camera->pos.pos.z;

    XYZ_32 pos = {};
    Collide_GetJointAbsPosition(item, &pos, 0);

    const int16_t room_num = Room_GetIndexFromPos(pos);
    if (room_num != NO_ROOM) {
        Item_UpdateRoom(item_num, room_num);
    }

    Lara_Animate(item);
}

void CutscenePlayer1_Initialise(const int16_t item_num)
{
    OBJECT *const obj = Object_Get(O_LARA);
    obj->draw_func = Lara_Draw;
    obj->control_func = Lara_Control_Cutscene;

    Item_AddActive(item_num);
    ITEM *const item = Item_Get(item_num);
    CAMERA_INFO *const camera = Cutscene_GetCamera();
    Camera_GetCineData()->position.target_angle = item->rot.y;
    g_Camera.target_angle = item->rot.y;
    camera->pos.pos.x = item->pos.x;
    camera->pos.pos.y = item->pos.y;
    camera->pos.pos.z = item->pos.z;
    camera->target_angle = item->rot.y;
    camera->pos.room_num = item->room_num;

    item->rot.y = 0;
    item->dynamic_light = false;
    item->goal_anim_state = 0;
    item->current_anim_state = 0;
    item->frame_num = 0;
    item->anim_num = 0;

    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->hit_direction = -1;
}
