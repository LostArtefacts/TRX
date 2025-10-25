#include "game/cutscene.h"

#include "game/effects.h"
#include "game/game.h"
#include "game/game_flow.h"
#include "game/room_draw.h"
#include "game/shell.h"

#include <libtrx/debug.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/input.h>
#include <libtrx/game/interpolation.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/level.h>
#include <libtrx/game/music.h>
#include <libtrx/game/output.h>
#include <libtrx/game/sound.h>
#include <libtrx/memory.h>

static CAMERA_INFO m_LocalCamera = {};

static void M_ControlLara(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    CAMERA_INFO *const camera = Cutscene_GetCamera();
    item->rot.y = camera->target_angle;
    item->pos = camera->pos.pos;

    XYZ_32 pos = {};
    Collide_GetJointAbsPosition(item, &pos, 0);

    int16_t room_num = Room_GetIndexFromPos(pos);
    if (room_num != NO_ROOM) {
        Item_UpdateRoom(item_num, room_num);
        const SECTOR *const sector =
            Room_GetSector(pos.x, pos.y, pos.z, &room_num);
        item->floor = Room_GetHeight(sector, pos.x, pos.y, pos.z);
    }

    Lara_Animate(item);
}

static void M_InitialiseLara(const GF_LEVEL *const level)
{
    OBJECT *const obj = Object_Get(O_LARA);
    obj->draw_func = Lara_Draw;
    obj->control_func = M_ControlLara;

    const int16_t item_num = Item_GetIndex(Lara_GetItem());
    Item_AddActive(item_num);
    ITEM *const item = Item_Get(item_num);

    Camera_GetCineData()->position.target_angle = item->rot.y;
    g_Camera.target_angle = item->rot.y;

    g_Camera.pos.room_num = item->room_num;
    CINE_DATA *const cine_data = Camera_GetCineData();
    cine_data->position.pos = item->pos;
    CAMERA_INFO *const camera = Cutscene_GetCamera();
    camera->pos.pos = item->pos;
    camera->pos.room_num = item->room_num;
    camera->target_angle = item->rot.y;

    item->rot.y = 0;
    item->dynamic_light = false;
    Item_SwitchToAnim(item, 0, 0);
    item->goal_anim_state = 0;
    item->current_anim_state = 0;

    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->hit_direction = DIR_UNKNOWN;
}

bool Cutscene_Start(const int32_t level_num)
{
    const GF_LEVEL *const level = GF_GetLevel(GFLT_CUTSCENES, level_num);
    ASSERT(GF_GetCurrentLevel() == level);

    M_InitialiseLara(level);

    const int32_t room_count = Room_GetCount();
    for (int16_t room_num = 0; room_num < room_count; room_num++) {
        const ROOM *const room = Room_Get(room_num);
        if (room->flipped_room >= 0) {
            Room_Get(room->flipped_room)->bound_active = 1;
        }
    }

    Room_DrawReset();
    for (int16_t room_num = 0; room_num < room_count; room_num++) {
        if (!Room_Get(room_num)->bound_active) {
            Room_MarkToBeDrawn(room_num);
        }
    }

    Camera_GetCineData()->frame_idx = 0;

    if (level->music_track != MX_INACTIVE) {
        Music_Play_Direct(level->music_track, MPM_ALWAYS);
    }

    return true;
}

void Cutscene_End(void)
{
    Music_Stop();
}

GF_COMMAND Cutscene_Control(void)
{
    Interpolation_Remember();
    Music_SyncTimestamp(Camera_GetCineData()->frame_idx / (double)LOGIC_FPS);

    Input_Update();
    Shell_ProcessInput();
    if (g_InputDB.menu_confirm || g_InputDB.menu_back) {
        return (GF_COMMAND) { .action = GF_LEVEL_COMPLETE };
    } else if (g_InputDB.pause) {
        const GF_COMMAND gf_cmd = GF_PauseGame();
        if (gf_cmd.action != GF_NOOP) {
            return gf_cmd;
        }
    } else if (g_InputDB.toggle_photo_mode) {
        const GF_COMMAND gf_cmd = GF_EnterPhotoMode();
        if (gf_cmd.action != GF_NOOP) {
            return gf_cmd;
        }
    }

    Output_ResetDynamicLights();

    Item_Control();
    Effect_Control();
    Lara_Hair_Control(true);
    Camera_UpdateCutscene();
    Output_AnimateTextures(1);

    CINE_DATA *const cine_data = Camera_GetCineData();
    cine_data->frame_idx++;
    if (cine_data->frame_idx >= cine_data->frame_count - 1) {
        // Remember the scene after the update to prevent the interpolation
        // from twitching the camera back and forth.
        Interpolation_Remember();

        cine_data->frame_idx = cine_data->frame_count - 2;
        return (GF_COMMAND) { .action = GF_LEVEL_COMPLETE };
    }

    return (GF_COMMAND) { .action = GF_NOOP };
}

void Cutscene_Draw(void)
{
    Game_Draw(true);
}

CAMERA_INFO *Cutscene_GetCamera(void)
{
    return &m_LocalCamera;
}
