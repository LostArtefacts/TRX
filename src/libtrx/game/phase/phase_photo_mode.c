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
#include "game/phase/executor.h"
#include "game/shell.h"
#include "game/sound.h"
#include "game/ui.h"
#include "memory.h"

#define M_INTERPOLATION_STEP 0.25

typedef struct {
    bool taking_screenshot;
    bool show_fps_counter;
    double rate;
} M_PRIV;

static PHASE_CONTROL M_Start(PHASE *phase);
static void M_End(PHASE *phase);
static PHASE_CONTROL M_Control(PHASE *phase, int32_t num_frames);
static void M_Draw(PHASE *phase);

static PHASE_CONTROL M_Start(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    p->show_fps_counter = g_Config.ui.enable_fps_counter;
    p->rate = 1.0;
    g_Config.ui.enable_fps_counter = false;

    Camera_EnterPhotoMode();
    Overlay_HideGameInfo();
    Music_Pause();
    Sound_PauseAll();

    if (!g_Config.ui.enable_photo_mode_ui) {
        Console_Log(GS(OSD_PHOTO_MODE_LAUNCHED));
    }
    return (PHASE_CONTROL) { .action = PHASE_ACTION_CONTINUE };
}

static void M_End(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    Camera_ExitPhotoMode();

    g_Config.ui.enable_fps_counter = p->show_fps_counter;
    Music_Unpause();
    Sound_UnpauseAll();
}

static PHASE_CONTROL M_Control(PHASE *const phase, int32_t num_frames)
{
    M_PRIV *const p = phase->priv;
    Interpolation_Remember();

    Input_Update();
    Shell_ProcessInput();

    PHASE_CONTROL result = (PHASE_CONTROL) { .action = PHASE_ACTION_CONTINUE };

    if (g_InputDB.pause) {
        PHASE *const phase = PhaseExecutor_GetOuterPhase();
        if (phase != nullptr && phase->control != nullptr
            && phase->draw != nullptr) {
            Camera_PausePhotoMode();
            const bool is_enabled = Interpolation_IsEnabled();
            const bool slow = g_Input.slow;
            Interpolation_Enable();
            if (phase->resume != nullptr) {
                phase->resume(phase);
            }
            if (p->rate >= 1.0) {
                g_Input.any = 0;
                g_InputDB.any = 0;
                result = phase->control(phase, 1);
                g_Input.any = 0;
                g_InputDB.any = 0;
                p->rate = 0.0;
            }
            p->rate += slow ? M_INTERPOLATION_STEP : 1.0f;
            Interpolation_SetRate(p->rate);
            phase->draw(phase);
            if (phase->suspend != nullptr) {
                phase->suspend(phase);
            }
            if (!is_enabled) {
                Interpolation_Disable();
            }
            Camera_ResumePhotoMode();
        }
    } else if (g_InputDB.toggle_ui) {
        UI_ToggleState(&g_Config.ui.enable_photo_mode_ui);
    } else if (g_InputDB.toggle_photo_mode || g_InputDB.menu_back) {
        result = (PHASE_CONTROL) {
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

    return result;
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
