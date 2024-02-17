#include "game/phase/phase_cutscene.h"

#include "game/camera.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/input.h"
#include "game/items.h"
#include "game/lara/lara_hair.h"
#include "game/music.h"
#include "game/output.h"
#include "game/shell.h"
#include "game/sound.h"
#include "global/types.h"
#include "global/vars.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static const int32_t m_CinematicAnimationRate = 0x8000;
static int32_t m_FrameCount = 0;

static void Phase_Cutscene_End(void);
static GAMEFLOW_OPTION Phase_Cutscene_Control(int32_t nframes);
static void Game_Cutscene_Draw(void);

static void Phase_Cutscene_End(void)
{
    Music_Stop();
    Sound_StopAllSamples();
}

static GAMEFLOW_OPTION Phase_Cutscene_Control(int32_t nframes)
{
    m_FrameCount += m_CinematicAnimationRate * nframes;
    while (m_FrameCount >= 0) {
        if (g_CineFrame >= g_NumCineFrames - 1) {
            g_LevelComplete = true;
            return GF_LEVEL_COMPLETE | g_CurrentLevel;
        }

        Input_Update();
        Shell_ProcessInput();
        Game_ProcessInput();

        if (g_InputDB.menu_confirm || g_InputDB.menu_back) {
            g_LevelComplete = true;
            return GF_LEVEL_COMPLETE | g_CurrentLevel;
        }

        Item_Control();
        Effect_Control();
        Lara_Hair_Control();
        Camera_UpdateCutscene();

        g_CineFrame++;
        m_FrameCount -= 0x10000;
    }

    return GF_NOP;
}

static void Phase_Cutscene_Draw(void)
{
    Game_DrawScene(true);
    Output_AnimateTextures(g_Camera.number_frames);
}

PHASER g_CutscenePhaser = {
    .start = NULL,
    .end = Phase_Cutscene_End,
    .control = Phase_Cutscene_Control,
    .draw = Phase_Cutscene_Draw,
};
