#include <trx/game/cutscene.h>

#include <trx/config.h>
#include <trx/debug.h>
#include <trx/game/camera.h>
#include <trx/game/effects.h>
#include <trx/game/footprint_fx.h>
#include <trx/game/interpolation.h>
#include <trx/game/lara.h>
#include <trx/game/lua.h>
#include <trx/game/music.h>
#include <trx/game/output.h>
#include <trx/game/shell.h>
#include <trx/game/water_fx.h>
#include <trx/game/weather_fx.h>
#include <trx/utils.h>

static CAMERA_INFO m_LocalCamera = {};

typedef struct {
    bool is_valid;
    LARA_SKIN_TYPE skin_type;
    LARA_GUN_TYPE hand_l_type;
    LARA_GUN_TYPE hand_r_type;
    LARA_GUN_TYPE thigh_l_type;
    LARA_GUN_TYPE thigh_r_type;
    bool holsters_visible;
} M_LARA_CUTSCENE_STATE;

static M_LARA_CUTSCENE_STATE m_LaraCutsceneState = {};

static LARA_GUN_TYPE M_GetGunEquipmentType(const LARA_MESH mesh)
{
    const LARA_SKIN_EQUIPMENT *const equipment = Lara_Skin_GetEquipment(mesh);
    if (equipment->type == EQUIPMENT_TYPE_WEAPON) {
        return (LARA_GUN_TYPE)equipment->data;
    }
    return LGT_UNARMED;
}

static void M_CaptureLaraCutsceneState(void)
{
    m_LaraCutsceneState.is_valid = true;
    m_LaraCutsceneState.skin_type = Lara_Skin_GetType();
    m_LaraCutsceneState.hand_l_type = M_GetGunEquipmentType(LM_HAND_L);
    m_LaraCutsceneState.hand_r_type = M_GetGunEquipmentType(LM_HAND_R);
    m_LaraCutsceneState.thigh_l_type = M_GetGunEquipmentType(LM_THIGH_L);
    m_LaraCutsceneState.thigh_r_type = M_GetGunEquipmentType(LM_THIGH_R);
    m_LaraCutsceneState.holsters_visible = Lara_Skin_AreHolstersVisible();
}

static void M_RestoreLaraCutsceneState(void)
{
    if (!m_LaraCutsceneState.is_valid) {
        return;
    }

    Lara_Skin_SetType(m_LaraCutsceneState.skin_type);
    Lara_Skin_SetGunEquipment(LM_HAND_L, m_LaraCutsceneState.hand_l_type);
    Lara_Skin_SetGunEquipment(LM_HAND_R, m_LaraCutsceneState.hand_r_type);
    Lara_Skin_SetGunEquipment(LM_THIGH_L, m_LaraCutsceneState.thigh_l_type);
    Lara_Skin_SetGunEquipment(LM_THIGH_R, m_LaraCutsceneState.thigh_r_type);
    Lara_Skin_SetHolstersVisible(m_LaraCutsceneState.holsters_visible);
}

static bool M_IsCutsceneActor(const ITEM *const item)
{
    return (item->object_id >= O_PLAYER_1 && item->object_id <= O_PLAYER_10)
        || item->object_id == O_LARA;
}

static void M_ResetActorAnimation(ITEM *const item)
{
    Item_SwitchToAnim(item, 0, 0);
    item->prev_frame_num = item->frame_num;
    item->current_anim_state = Item_GetAnim(item)->current_anim_state;
    item->goal_anim_state = item->current_anim_state;
    item->required_anim_state = 0;
}

static void M_AnimateActors(void)
{
    for (int32_t i = 0; i < Item_GetTotalCount(); i++) {
        ITEM *const item = Item_Get(i);
        if (!M_IsCutsceneActor(item)) {
            continue;
        }

        const OBJECT *const obj = Object_Get(item->object_id);
        if (obj->control_func != nullptr) {
            obj->control_func(i);
        }
    }
}

static void M_ResetActorsToStart(void)
{
    M_RestoreLaraCutsceneState();

    for (int32_t i = 0; i < Item_GetTotalCount(); i++) {
        ITEM *const item = Item_Get(i);
        if (M_IsCutsceneActor(item)) {
            M_ResetActorAnimation(item);
        }
    }
}

static void M_ReplayActors(
    CINE_DATA *const cine_data, const int32_t start_frame,
    const int32_t end_frame)
{
    for (int32_t frame_idx = start_frame; frame_idx < end_frame; frame_idx++) {
        Lua_FireEventInt32(LUA_EVENT_BEFORE_CONTROL, 0);
        cine_data->frame_idx = frame_idx;
        Camera_UpdateCutscene();
        M_AnimateActors();
        Lua_FireEventInt32(LUA_EVENT_AFTER_CONTROL, 0);
    }
}

static void M_PlayerControl(const int16_t item_num)
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
    const int32_t source_frame = cine_data->frame_idx;
    int32_t target_frame = source_frame + frames;
    CLAMP(target_frame, 0, cine_data->frame_count - 1);

    if (target_frame == source_frame) {
        return;
    }

    if (target_frame > source_frame) {
        M_ReplayActors(cine_data, source_frame, target_frame);
    } else {
        Lua_ReloadLevelScript();
        M_ResetActorsToStart();
        M_ReplayActors(cine_data, 0, target_frame);
    }

    cine_data->frame_idx = target_frame;
    Camera_UpdateCutscene();
}

bool Cutscene_Start(const int32_t level_num)
{
    const GF_LEVEL *const level = GF_GetLevel(GFLT_CUTSCENES, level_num);
    ASSERT(GF_GetCurrentLevel() == level);

    M_InitialisePlayer(Item_GetIndex(Lara_GetItem()));
    M_CaptureLaraCutsceneState();
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
    } else if (g_InputDB.menu_right || g_InputDB.menu_left) {
        const int32_t dir = g_InputDB.menu_right ? 1 : -1;
        const int32_t speed = g_Input.draw ? 15 : (g_Input.slow ? 1 : 5);
        M_Skip(dir * LOGIC_FPS * speed);
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
