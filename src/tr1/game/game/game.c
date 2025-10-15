#include "game/game.h"

#include "game/effects.h"
#include "game/game_flow.h"
#include "game/inventory.h"
#include "game/item_actions.h"
#include "game/lara.h"
#include "game/savegame.h"
#include "game/shell.h"
#include "game/stats.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/clock.h>
#include <libtrx/game/input.h>
#include <libtrx/game/interpolation.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/level.h>
#include <libtrx/game/music.h>
#include <libtrx/game/output.h>
#include <libtrx/game/overlay.h>
#include <libtrx/game/sound.h>
#include <libtrx/game/ui.h>

#define FRAME_BUFFER(key)                                                      \
    do {                                                                       \
        Shell_ProcessEvents();                                                 \
        Output_BeginScene();                                                   \
        Game_Draw(true);                                                       \
        Input_Update();                                                        \
        Output_EndScene();                                                     \
        Output_FlipScreen();                                                   \
        Clock_WaitTick();                                                      \
    } while (g_Input.key);

void Game_ProcessInput(void)
{
    if (g_InputDB.use_small_medi && Inv_RequestItem(O_SMALL_MEDIPACK_OPTION)) {
        Lara_UseItem(O_SMALL_MEDIPACK_OPTION);
    } else if (
        g_InputDB.use_big_medi && Inv_RequestItem(O_LARGE_MEDIPACK_OPTION)) {
        Lara_UseItem(O_LARGE_MEDIPACK_OPTION);
    }

    if (g_Config.input.enable_buffering && Game_IsPlaying()) {
        if (g_Input.toggle_bilinear_filter) {
            FRAME_BUFFER(toggle_bilinear_filter);
        } else if (g_Input.toggle_trapezoid_filter) {
            FRAME_BUFFER(toggle_trapezoid_filter);
        } else if (g_Input.toggle_fps_counter) {
            FRAME_BUFFER(toggle_fps_counter);
        }
    }

    if (g_InputDB.toggle_ui) {
        UI_ToggleState(&g_Config.ui.enable_game_ui);
    }
}

bool Game_Start(const GF_LEVEL *const level, const GF_SEQUENCE_CONTEXT seq_ctx)
{
    Game_SetCurrentLevel(level);
    Game_FadeToBlack(-1);

    g_OverlayFlag = 1;
    Camera_Initialise();
    Interpolation_Remember();

    Sound_StopAll();
    const bool is_cutscene = level->type == GFL_CUTSCENE;
    if (level->music_track != MX_INACTIVE
        && (is_cutscene || Music_GetCurrentLoopedTrack() == MX_INACTIVE)) {
        Music_Play_Direct(
            level->music_track, is_cutscene ? MPM_ALWAYS : MPM_LOOPED);
    }

    return true;
}

void Game_End(void)
{
    Savegame_PersistGameToCurrentInfo(Game_GetCurrentLevel());
    Music_Stop();
}

GF_COMMAND Game_Control(const bool demo_mode)
{
    ASSERT(!demo_mode);

    if (g_GameInfo.ask_for_save) {
        // ask for a save at the start of a level for the save crystals mode
        const GF_COMMAND gf_cmd = GF_ShowInventory(INV_SAVE_CRYSTAL_MODE);
        g_GameInfo.ask_for_save = false;
        if (gf_cmd.action != GF_NOOP) {
            return gf_cmd;
        }
    }

    Interpolation_Remember();
    Stats_UpdateTimer();

    Lara_Cheat_CheckKeys();
    if (Game_IsLevelComplete()) {
        Sound_StopAll();
        Music_Stop();
        return (GF_COMMAND) { .action = GF_LEVEL_COMPLETE };
    }

    Input_Update();
    Shell_ProcessInput();
    Game_ProcessInput();

    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->death_timer > DEATH_WAIT
        || (lara->death_timer > DEATH_WAIT_INPUT
            && (g_InputDB.menu_confirm || g_InputDB.menu_back)
            && !g_Input.fly_cheat)
        || g_OverlayFlag == 2) {
        if (g_OverlayFlag == 2) {
            g_OverlayFlag = 1;
            return GF_ShowInventory(INV_DEATH_MODE);
        } else {
            g_OverlayFlag = 2;
        }
    }

    if ((g_InputDB.option || g_Input.save || g_Input.load || g_OverlayFlag <= 0)
        && !lara->death_timer) {
        if (g_Camera.type == CAM_CINEMATIC) {
            g_OverlayFlag = 0;
        } else if (g_OverlayFlag > 0) {
            if (g_Input.save) {
                g_OverlayFlag = -2;
            } else if (g_Input.load) {
                g_OverlayFlag = -1;
            } else {
                g_OverlayFlag = 0;
            }
        } else {
            GF_COMMAND gf_cmd;
            if (g_OverlayFlag == -1) {
                gf_cmd = GF_ShowInventory(INV_LOAD_MODE);
            } else if (g_OverlayFlag == -2) {
                gf_cmd = GF_ShowInventory(INV_SAVE_MODE);
            } else {
                gf_cmd = GF_ShowInventory(INV_GAME_MODE);
            }
            g_OverlayFlag = 1;
            return gf_cmd;
        }
    }

    if (!lara->death_timer && g_InputDB.pause) {
        return GF_PauseGame();
    } else if (g_InputDB.toggle_photo_mode) {
        return GF_EnterPhotoMode();
    } else {
        Output_ResetDynamicLights();

        Sound_ResetAmbient();
        Item_Control();
        Effect_Control();

        Lara_Control();
        Lara_Hair_Control(false);

        Camera_Update();
        ItemAction_RunActive();
        Sound_UpdateEffects();
        Overlay_Animate(1);
        Output_AnimateTextures(1);
    }
    return (GF_COMMAND) { .action = GF_NOOP };
}
