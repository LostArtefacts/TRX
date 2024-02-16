#include "game/phase/phase_game.h"

#include "config.h"
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
#include "game/savegame.h"
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
    Output_FadeReset();
}

static GAMEFLOW_OPTION Phase_Game_Control(int32_t nframes)
{
    int32_t return_val = 0;
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
                return_val = Inv_Display(INV_DEATH_MODE);
                if (return_val != GF_NOP) {
                    return return_val;
                }
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
                    return_val = Inv_Display(INV_LOAD_MODE);
                } else if (g_OverlayFlag == -2) {
                    return_val = Inv_Display(INV_SAVE_MODE);
                } else {
                    return_val = Inv_Display(INV_GAME_MODE);
                }

                g_OverlayFlag = 1;
                if (return_val != GF_NOP) {
                    return return_val;
                }
            }
        }

        if (!g_Lara.death_timer && g_InputDB.pause) {
            Game_SetStatus(GS_IN_PAUSE);
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
        int32_t return_val = Inv_Display(INV_SAVE_CRYSTAL_MODE);
        if (return_val != GF_NOP) {
            Savegame_Save(g_GameInfo.current_save_slot, &g_GameInfo);
            Config_Write();
        }
        g_GameInfo.ask_for_save = false;
    }

    return GF_NOP;
}

static void Phase_Game_Draw(void)
{
    Game_DrawScene(true);
    Text_Draw();
    Output_AnimateTextures(g_Camera.number_frames);
    Text_Draw();
}

PHASER g_GamePhaser = {
    .start = Phase_Game_Start,
    .end = NULL,
    .control = Phase_Game_Control,
    .draw = Phase_Game_Draw,
};
