#include "game/phase/phase_photo_mode.h"

#include "config.h"
#include "game/camera.h"
#include "game/game.h"
#include "game/input.h"
#include "game/interpolation.h"
#include "game/music.h"
#include "game/overlay.h"
#include "game/shell.h"
#include "game/sound.h"
#include "game/ui/widgets/photo_mode.h"

#include <libtrx/game/console/common.h>
#include <libtrx/game/game_string.h>
#include <libtrx/game/ui/common.h>

#include <assert.h>
#include <stdio.h>

typedef enum {
    PS_NONE,
    PS_ACTIVE,
    PS_COOLDOWN,
} PHOTO_STATUS;

static PHASE_PHOTO_MODE_ARGS m_Args;

static PHOTO_STATUS m_Status = PS_NONE;
static UI_WIDGET *m_PhotoMode = NULL;

static void M_Start(const PHASE_PHOTO_MODE_ARGS *args);
static void M_End(void);
static PHASE_CONTROL M_Control(int32_t nframes);
static void M_Draw(void);

static void M_Start(const PHASE_PHOTO_MODE_ARGS *const args)
{
    assert(args != NULL);
    m_Args = *args;

    m_Status = PS_NONE;
    g_OldInputDB = g_Input;
    Camera_EnterPhotoMode();

    Overlay_HideGameInfo();
    Music_Pause();
    Sound_PauseAll();

    m_PhotoMode = UI_PhotoMode_Create();
    if (!g_Config.ui.enable_photo_mode_ui) {
        Console_Log(
            GS(OSD_PHOTO_MODE_LAUNCHED),
            Input_GetKeyName(
                CM_KEYBOARD, g_Config.input.layout, INPUT_ROLE_TOGGLE_UI));
    }
}

static void M_End(void)
{
    Camera_ExitPhotoMode();

    g_Input = g_OldInputDB;

    m_PhotoMode->free(m_PhotoMode);
    m_PhotoMode = NULL;

    Music_Unpause();
    Sound_UnpauseAll();
}

static PHASE_CONTROL M_Control(int32_t nframes)
{
    if (m_Status == PS_ACTIVE) {
        Shell_MakeScreenshot();
        Sound_Effect(SFX_MENU_CHOOSE, NULL, SPM_ALWAYS);
        m_Status = PS_COOLDOWN;
    } else if (m_Status == PS_COOLDOWN) {
        m_Status = PS_NONE;
    }

    Input_Update();
    Shell_ProcessInput();

    if (g_InputDB.toggle_ui) {
        UI_ToggleState(&g_Config.ui.enable_photo_mode_ui);
    }

    if (g_InputDB.toggle_photo_mode || g_InputDB.option) {
        Phase_Set(m_Args.phase_to_return_to, m_Args.phase_arg);
        return (PHASE_CONTROL) { .end = false };
    } else if (g_InputDB.action) {
        m_Status = PS_ACTIVE;
    } else {
        m_PhotoMode->control(m_PhotoMode);
        Camera_Update();
    }

    return (PHASE_CONTROL) { .end = false };
}

static void M_Draw(void)
{
    Interpolation_Disable();
    Game_DrawScene(false);
    Interpolation_Enable();

    if (m_Status == PS_NONE) {
        m_PhotoMode->draw(m_PhotoMode);
    }
}

PHASER g_PhotoModePhaser = {
    .start = (PHASER_START)M_Start,
    .end = M_End,
    .control = M_Control,
    .draw = M_Draw,
    .wait = NULL,
};
