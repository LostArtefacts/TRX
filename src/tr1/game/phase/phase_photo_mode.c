#include "game/phase/phase_photo_mode.h"

#include "game/camera.h"
#include "game/game.h"
#include "game/input.h"
#include "game/interpolation.h"
#include "game/music.h"
#include "game/overlay.h"
#include "game/shell.h"
#include "game/sound.h"
#include "game/viewport.h"

#include <libtrx/game/console/common.h>

#include <stdio.h>

#define MIN_PHOTO_FOV 10
#define MAX_PHOTO_FOV 150

typedef enum {
    PS_NONE,
    PS_ACTIVE,
    PS_COOLDOWN,
} PHOTO_STATUS;

static int32_t m_OldFOV;
static int32_t m_CurrentFOV;

static PHOTO_STATUS m_Status = PS_NONE;

static void M_Start(void *arg);
static void M_End(void);
static PHASE_CONTROL M_Control(int32_t nframes);
static void M_Draw(void);
static void M_AdjustFOV(void);

static void M_Start(void *arg)
{
    m_Status = PS_NONE;
    g_OldInputDB = g_Input;
    m_OldFOV = Viewport_GetFOV();
    m_CurrentFOV = m_OldFOV / PHD_DEGREE;

    Overlay_HideGameInfo();
    Music_Pause();
    Sound_PauseAll();
}

static void M_End(void)
{
    g_Input = g_OldInputDB;
    Viewport_SetFOV(m_OldFOV);

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

    if (g_InputDB.toggle_photo_mode || g_InputDB.menu_back) {
        Phase_Set(PHASE_GAME, NULL);
    } else if (g_InputDB.action) {
        m_Status = PS_ACTIVE;
    } else {
        M_AdjustFOV();
        Camera_Update();
    }

    return (PHASE_CONTROL) { .end = false };
}

static void M_AdjustFOV(void)
{
    if (g_InputDB.look) {
        Viewport_SetFOV(m_OldFOV);
        m_CurrentFOV = m_OldFOV / PHD_DEGREE;
        return;
    }

    if (g_InputDB.toggle_ui) {
        // This needs to be re-applied as Config_Write() will have reset it.
        Viewport_SetFOV(m_CurrentFOV * PHD_DEGREE);
    }

    if (!g_Input.draw) {
        return;
    }

    if (g_Input.slow) {
        m_CurrentFOV--;
    } else {
        m_CurrentFOV++;
    }
    CLAMP(m_CurrentFOV, MIN_PHOTO_FOV, MAX_PHOTO_FOV);
    Viewport_SetFOV(m_CurrentFOV * PHD_DEGREE);
}

static void M_Draw(void)
{
    Interpolation_Disable();
    Game_DrawScene(false);
    Interpolation_Enable();
}

PHASER g_PhotoModePhaser = {
    .start = M_Start,
    .end = M_End,
    .control = M_Control,
    .draw = M_Draw,
    .wait = NULL,
};
