#include <trx/game/cutscene.h>

#include <trx/config.h>
#include <trx/debug.h>
#include <trx/game/camera.h>
#include <trx/game/effects.h>
#include <trx/game/footprint_fx.h>
#include <trx/game/interpolation.h>
#include <trx/game/lara.h>
#include <trx/game/music.h>
#include <trx/game/output.h>
#include <trx/game/shell.h>
#include <trx/game/water_fx.h>
#include <trx/game/weather_fx.h>

static CAMERA_INFO m_LocalCamera = {};

static void M_PlayerControl(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    CAMERA_INFO *const camera = Cutscene_GetCamera();
    item->rot.y = camera->target_angle;
    item->pos.x = camera->pos.pos.x;
    item->pos.y = camera->pos.pos.y;
    item->pos.z = camera->pos.pos.z;

    XYZ_32 pos = {};
    Collide_GetJointAbsPosition(item, &pos, 0);

    int16_t room_num = Room_GetIndexFromPos(pos);
    if (room_num != NO_ROOM) {
        Item_UpdateRoom(item_num, room_num);
    }

    Lara_Animate(item);
}

static void M_InitialisePlayer(const int16_t item_num)
{
    OBJECT *const obj = Object_Get(O_LARA);
    obj->draw_func = Lara_Draw;
    obj->control_func = M_PlayerControl;

    Item_AddActive(item_num);
    ITEM *const item = Item_Get(item_num);
    CAMERA_INFO *const camera = Cutscene_GetCamera();
    Camera_GetCineData()->position.target_angle = item->rot.y;
    g_Camera.pos.room_num = item->room_num;
    g_Camera.target_angle = item->rot.y;

    CINE_DATA *const cine_data = Camera_GetCineData();
    cine_data->position.pos = item->pos;

    camera->pos.pos = item->pos;
    if (item->room_num != NO_ROOM) {
        camera->pos.room_num = item->room_num;
    }
    camera->target_angle = item->rot.y;

    item->rot.y = 0;
    item->dynamic_light = false;
    Item_SwitchToAnim(item, 0, 0);
    item->goal_anim_state = 0;
    item->current_anim_state = 0;

    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->hit_direction = DIR_UNKNOWN;
}

static void M_Skip(const int32_t frames)
{
    CINE_DATA *const cine_data = Camera_GetCineData();
    cine_data->frame_idx += frames;
    if (cine_data->frame_idx >= cine_data->frame_count) {
        cine_data->frame_idx = cine_data->frame_count - 1;
    }
    for (int32_t i = 0; i < frames; i++) {
        for (int32_t j = 0; j < Item_GetTotalCount(); j++) {
            ITEM *const item = Item_Get(j);
            if ((item->object_id >= O_PLAYER_1
                 && item->object_id <= O_PLAYER_10)
                || item->object_id == O_LARA) {
                Item_Animate(item);
            }
        }
    }
}

bool Cutscene_Start(const int32_t level_num)
{
    const GF_LEVEL *const level = GF_GetLevel(GFLT_CUTSCENES, level_num);
    ASSERT(GF_GetCurrentLevel() == level);

    M_InitialisePlayer(Item_GetIndex(Lara_GetItem()));
    Camera_GetCineData()->frame_idx = 0;

    if (level->music_track != MX_INACTIVE) {
        Music_Play_Direct(level->music_track, MPM_ONCE);
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
    } else if (g_InputDB.menu_right) {
        M_Skip(g_InputDB.slow ? LOGIC_FPS : LOGIC_FPS * 5);
    }

    Output_ResetDynamicLights();

    Item_Control();
    Effect_Control();
    Lara_Hair_Control(true);
    Camera_UpdateCutscene();
    WaterFX_Update();
    WeatherFX_Update();
    FootprintFX_Update();
    Output_AnimateTextures(1);

    CINE_DATA *const cine_data = Camera_GetCineData();
    cine_data->frame_idx++;
    if (cine_data->frame_idx >= cine_data->frame_count) {
        // Remember the scene after the update to prevent the interpolation
        // from twitching the camera back and forth.
        Interpolation_Remember();

        return (GF_COMMAND) { .action = GF_LEVEL_COMPLETE };
    }

    return (GF_COMMAND) { .action = GF_NOOP };
}

void Cutscene_Draw(void)
{
    Interpolation_Interpolate();
    Camera_Apply();
    Room_DrawAllRooms(g_Camera.interp.room_num, g_Camera.target.room_num);
    SceneCompositor_Flush();
    if (g_Config.visuals.enable_reflections) {
        Output_Textures_UpdateEnvironmentMap();
    }
}

CAMERA_INFO *Cutscene_GetCamera(void)
{
    return &m_LocalCamera;
}
