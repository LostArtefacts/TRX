#include "game/phase/phase_game.h"

#include "game/camera.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/input.h"
#include "game/inventory.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/lara/lara_cheat.h"
#include "game/lara/lara_hair.h"
#include "game/output.h"
#include "game/overlay.h"
#include "game/shell.h"
#include "game/sound.h"
#include "game/text.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static const int32_t m_AnimationRate = 0x8000;
static int32_t m_FrameCount = 0;

static void Phase_Game_Start(void *arg);
static GAMEFLOW_OPTION Phase_Game_Control(int32_t nframes);
static void Phase_Game_Draw(void);

static void Phase_Game_Start(void *arg)
{
    if (Phase_Get() != PHASE_PAUSE) {
        Output_FadeReset();
        Output_FadeSetSpeed(1.0);
    }
}

static GAMEFLOW_OPTION Phase_Game_Control(int32_t nframes)
{
    GAMEFLOW_OPTION return_val = GF_NOP;
    if (nframes > MAX_FRAMES) {
        nframes = MAX_FRAMES;
    }

    m_FrameCount += m_AnimationRate * nframes;
    while (m_FrameCount >= 0) {
        Lara_CheckCheatMode();
        if (g_LevelComplete) {
            return GF_NOP_BREAK;
        }

        Input_Update();
        Shell_ProcessInput();
        Game_ProcessInput();

        if (g_GameInfo.current_level_type == GFL_DEMO) {
            if (g_Input.any) {
                return GF_EXIT_TO_TITLE;
            }
            if (!Game_Demo_ProcessInput()) {
                return GF_EXIT_TO_TITLE;
            }
        }

        if (g_Lara.death_timer > DEATH_WAIT
            || (g_Lara.death_timer > DEATH_WAIT_MIN && g_Input.any
                && !g_Input.fly_cheat)
            || g_OverlayFlag == 2) {
            if (g_GameInfo.current_level_type == GFL_DEMO) {
                return GF_EXIT_TO_TITLE;
            }
            if (g_OverlayFlag == 2) {
                g_OverlayFlag = 1;
                Inv_Display(INV_DEATH_MODE);
                return GF_NOP;
            } else {
                g_OverlayFlag = 2;
            }
        }

        if ((g_InputDB.option || g_Input.save || g_Input.load
             || g_OverlayFlag <= 0)
            && !g_Lara.death_timer) {
            if (g_Camera.type == CAM_CINEMATIC) {
                g_OverlayFlag = 0;
            } else if (g_OverlayFlag > 0) {
                if (g_Input.load) {
                    g_OverlayFlag = -1;
                } else if (g_Input.save) {
                    g_OverlayFlag = -2;
                } else {
                    g_OverlayFlag = 0;
                }
            } else {
                if (g_OverlayFlag == -1) {
                    Inv_Display(INV_LOAD_MODE);
                } else if (g_OverlayFlag == -2) {
                    Inv_Display(INV_SAVE_MODE);
                } else {
                    Inv_Display(INV_GAME_MODE);
                }

                g_OverlayFlag = 1;
                return GF_NOP;
            }
        }

        if (!g_Lara.death_timer && g_InputDB.pause) {
            Phase_Set(PHASE_PAUSE, NULL);
            return GF_NOP;
        } else {
            Item_Control();
            Effect_Control();

            Lara_Control();
            Lara_Hair_Control();

            Camera_Update();
            Sound_ResetAmbient();
            Effect_RunActiveFlipEffect();
            Sound_UpdateEffects();
            g_GameInfo.current[g_CurrentLevel].stats.timer++;
            Overlay_BarHealthTimerTick();
        }

        m_FrameCount -= 0x10000;
    }

    if (g_GameInfo.ask_for_save) {
        Inv_Display(INV_SAVE_CRYSTAL_MODE);
        g_GameInfo.ask_for_save = false;
    }

    return GF_NOP;
}

static void Phase_Game_Draw(void)
{
    Game_DrawScene(true);
    Output_AnimateTextures(g_Camera.number_frames);
    Text_Draw();
}

PHASER g_GamePhaser = {
    .start = Phase_Game_Start,
    .end = NULL,
    .control = Phase_Game_Control,
    .draw = Phase_Game_Draw,
    .wait = NULL,
};
