#include "game/phase/phase_game.h"

#include "game/camera.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/input.h"
#include "game/interpolation.h"
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

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static void Phase_Game_Start(void *arg);
static void Phase_Game_End(void);
static GAMEFLOW_OPTION Phase_Game_Control(int32_t nframes);
static void Phase_Game_Draw(void);

static void Phase_Game_Start(void *arg)
{
    Interpolation_Enable();
    Interpolation_Remember();
    if (Phase_Get() != PHASE_PAUSE) {
        Output_FadeReset();
    }
}

static void Phase_Game_End(void)
{
    Interpolation_Disable();
}

static GAMEFLOW_OPTION Phase_Game_Control(int32_t nframes)
{
    Interpolation_Remember();
    CLAMPG(nframes, MAX_FRAMES);

    for (int32_t i = 0; i < nframes; i++) {
        Lara_CheckCheatMode();
        if (g_LevelComplete) {
            return GF_PHASE_BREAK;
        }

        Input_Update();
        Shell_ProcessInput();
        Game_ProcessInput();

        if (g_Lara.death_timer > DEATH_WAIT
            || (g_Lara.death_timer > DEATH_WAIT_MIN && g_Input.any
                && !g_Input.fly_cheat)
            || g_OverlayFlag == 2) {
            if (g_OverlayFlag == 2) {
                g_OverlayFlag = 1;
                Inv_Display(INV_DEATH_MODE);
                return GF_PHASE_CONTINUE;
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
                return GF_PHASE_CONTINUE;
            }
        }

        if (!g_Lara.death_timer && g_InputDB.pause) {
            Phase_Set(PHASE_PAUSE, NULL);
            return GF_PHASE_CONTINUE;
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
    }

    if (g_GameInfo.ask_for_save) {
        Inv_Display(INV_SAVE_CRYSTAL_MODE);
        g_GameInfo.ask_for_save = false;
    }

    return GF_PHASE_CONTINUE;
}

static void Phase_Game_Draw(void)
{
    Game_DrawScene(true);
    Output_AnimateTextures();
    Output_AnimateFades();
    Text_Draw();
}

PHASER g_GamePhaser = {
    .start = Phase_Game_Start,
    .end = Phase_Game_End,
    .control = Phase_Game_Control,
    .draw = Phase_Game_Draw,
    .wait = NULL,
};
