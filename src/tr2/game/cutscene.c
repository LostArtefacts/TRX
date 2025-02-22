#include "game/cutscene.h"

#include "decomp/decomp.h"
#include "game/camera.h"
#include "game/effects.h"
#include "game/game_flow.h"
#include "game/items.h"
#include "game/lara/hair.h"
#include "game/level.h"
#include "game/music.h"
#include "game/output.h"
#include "game/room.h"
#include "game/room_draw.h"
#include "game/shell.h"
#include "game/sound.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/game/interpolation.h>
#include <libtrx/utils.h>

static CAMERA_INFO m_LocalCamera = {};

static void M_FixAudioDrift(void);

static void M_FixAudioDrift(void)
{
    const int32_t audio_frame_idx = Music_GetTimestamp() * FRAMES_PER_SECOND;
    const int32_t game_frame_idx = Camera_GetCineData()->frame_idx;
    const int32_t audio_drift = ABS(audio_frame_idx - game_frame_idx);
    if (audio_drift >= FRAMES_PER_SECOND * 0.2) {
        LOG_DEBUG("Detected audio drift: %d frames", audio_drift);
        Music_SeekTimestamp(game_frame_idx / (double)FRAMES_PER_SECOND);
    }
}

bool Cutscene_Start(const int32_t level_num)
{
    const GF_LEVEL *const level = GF_GetLevel(GFLT_CUTSCENES, level_num);
    ASSERT(GF_GetCurrentLevel() == level);

    Room_InitCinematic();
    CutscenePlayer1_Initialise(g_Lara.item_num);
    CINE_DATA *const cine_data = Camera_GetCineData();
    g_Camera.target_angle = cine_data->position.target_angle;

    Music_SetVolume(10);
    cine_data->frame_idx = 0;
    return true;
}

void Cutscene_End(void)
{
    Music_SetVolume(g_Config.audio.music_volume);
    Music_Stop();
    Sound_StopAll();
}

GF_COMMAND Cutscene_Control(void)
{
    Interpolation_Remember();
    M_FixAudioDrift();

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
    if (cine_data->frame_idx >= cine_data->frame_count) {
        return (GF_COMMAND) { .action = GF_LEVEL_COMPLETE };
    }

    return (GF_COMMAND) { .action = GF_NOOP };
}

void Cutscene_Draw(void)
{
    g_CameraUnderwater = false;
    Interpolation_Commit();
    Camera_Apply();
    Room_DrawAllRooms(g_Camera.interp.room_num);
    Output_DrawPolyList();
}

CAMERA_INFO *Cutscene_GetCamera(void)
{
    return &m_LocalCamera;
}
