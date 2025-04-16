#include "game/phase/phase_photo_mode.h"

#include "config.h"
#include "game/camera.h"
#include "game/console/common.h"
#include "game/game.h"
#include "game/game_string.h"
#include "game/input.h"
#include "game/interpolation.h"
#include "game/music.h"
#include "game/output.h"
#include "game/overlay.h"
#include "game/shell.h"
#include "game/sound.h"
#include "game/ui.h"
#include "memory.h"

typedef struct {
    bool taking_screenshot;
    bool show_fps_counter;
} M_PRIV;

static PHASE_CONTROL M_Start(PHASE *phase);
static void M_End(PHASE *phase);
static PHASE_CONTROL M_Control(PHASE *phase, int32_t num_frames);
static void M_Draw(PHASE *phase);

static PHASE_CONTROL M_Start(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
#if TR_VERSION == 1
    p->show_fps_counter = g_Config.rendering.enable_fps_counter;
    g_Config.rendering.enable_fps_counter = false;
#endif

    Camera_EnterPhotoMode();
    Overlay_HideGameInfo();
    Music_Pause();
    Sound_PauseAll();

    if (!g_Config.ui.enable_photo_mode_ui) {
        Console_Log(
            GS(OSD_PHOTO_MODE_LAUNCHED),
            Input_GetKeyName(
                INPUT_BACKEND_KEYBOARD, g_Config.input.keyboard_layout,
                INPUT_ROLE_TOGGLE_UI));
    }
    return (PHASE_CONTROL) { .action = PHASE_ACTION_CONTINUE };
}

static void M_End(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    Camera_ExitPhotoMode();

#if TR_VERSION == 1
    g_Config.rendering.enable_fps_counter = p->show_fps_counter;
#endif
    Music_Unpause();
    Sound_UnpauseAll();
}

static PHASE_CONTROL M_Control(PHASE *const phase, int32_t num_frames)
{
    M_PRIV *const p = phase->priv;
    Interpolation_Remember();

    Input_Update();
    Shell_ProcessInput();

    if (g_InputDB.toggle_ui) {
        UI_ToggleState(&g_Config.ui.enable_photo_mode_ui);
    }

    if (g_InputDB.toggle_photo_mode || g_InputDB.menu_back) {
        return (PHASE_CONTROL) {
            .action = PHASE_ACTION_END,
            .gf_cmd = { .action = GF_NOOP },
        };
    } else if (g_InputDB.action) {
        Output_BeginScene();
        p->taking_screenshot = true;
        M_Draw(phase);
        p->taking_screenshot = false;
        Screenshot_Make(g_Config.rendering.screenshot_format);
        Output_EndScene();
        Sound_Effect(SFX_MENU_LARA_HOME, nullptr, SPM_ALWAYS);
    } else {
        Camera_Update();
    }

    return (PHASE_CONTROL) { .action = PHASE_ACTION_CONTINUE };
}

static void M_Draw(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    Game_Draw(false);
    Output_DrawPolyList();

    if (!p->taking_screenshot) {
        UI_PhotoMode();
    }
    Output_DrawPolyList();
}

PHASE *Phase_PhotoMode_Create(void)
{
    PHASE *const phase = Memory_Alloc(sizeof(PHASE));
    phase->priv = Memory_Alloc(sizeof(M_PRIV));
    phase->start = M_Start;
    phase->end = M_End;
    phase->control = M_Control;
    phase->draw = M_Draw;
    return phase;
}

void Phase_PhotoMode_Destroy(PHASE *phase)
{
    Memory_Free(phase->priv);
    Memory_Free(phase);
}
