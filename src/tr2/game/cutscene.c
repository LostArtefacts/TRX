#include "game/cutscene.h"

#include "decomp/decomp.h"
#include "game/effects.h"
#include "game/game_flow.h"
#include "game/level.h"
#include "game/room_draw.h"
#include "game/shell.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/interpolation.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/lara/hair.h>
#include <libtrx/game/music.h>
#include <libtrx/game/output.h>
#include <libtrx/game/sound.h>
#include <libtrx/utils.h>

static CAMERA_INFO m_LocalCamera = {};

bool Cutscene_Start(const int32_t level_num)
{
    const GF_LEVEL *const level = GF_GetLevel(GFLT_CUTSCENES, level_num);
    ASSERT(GF_GetCurrentLevel() == level);

    CutscenePlayer1_Initialise(Item_GetIndex(Lara_GetItem()));
    CINE_DATA *const cine_data = Camera_GetCineData();
    g_Camera.target_angle = cine_data->position.target_angle;

    cine_data->frame_idx = 0;

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
    Room_DrawAllRooms(g_Camera.interp.room_num);
    SceneCompositor_Flush();
}

CAMERA_INFO *Cutscene_GetCamera(void)
{
    return &m_LocalCamera;
}
