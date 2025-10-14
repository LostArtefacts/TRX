#include "game/game.h"

#include "decomp/decomp.h"
#include "game/demo.h"
#include "game/effects.h"
#include "game/game_flow.h"
#include "game/inventory.h"
#include "game/item_actions.h"
#include "game/lara.h"
#include "game/level.h"
#include "game/room_draw.h"
#include "game/savegame.h"
#include "game/shell.h"
#include "game/stats.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/gym.h>
#include <libtrx/game/interpolation.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/music.h>
#include <libtrx/game/output.h>
#include <libtrx/game/overlay.h>
#include <libtrx/game/sound.h>

bool Game_Start(const GF_LEVEL *const level, const GF_SEQUENCE_CONTEXT seq_ctx)
{
    Game_SetCurrentLevel(level);

    g_OverlayFlag = 1;
    Camera_Initialise();
    Interpolation_Remember();

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
    Interpolation_Remember();
    if (g_GameFlow.cheat_keys) {
        Lara_Cheat_CheckKeys();
    }

    if (Game_IsLevelComplete()) {
        return (GF_COMMAND) { .action = GF_LEVEL_COMPLETE };
    }

    Input_Update();
    Shell_ProcessInput();
    Game_ProcessInput();

    if (g_InputDB.toggle_photo_mode) {
        return GF_EnterPhotoMode();
    }
    if (g_InputDB.pause) {
        return GF_PauseGame();
    }

    if (demo_mode) {
        if (g_InputDB.menu_confirm || g_InputDB.menu_back) {
            return g_GameFlow.cmd_demo_interrupt;
        }
        if (!Demo_GetInput()) {
            g_Input = (INPUT_STATE) {};
            return g_GameFlow.cmd_demo_end;
        }
    }

    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->death_timer > DEATH_WAIT
        || (lara->death_timer > DEATH_WAIT_INPUT
            && (g_InputDB.menu_confirm || g_InputDB.menu_back))
        || g_OverlayFlag == 2) {
        if (demo_mode) {
            return g_GameFlow.cmd_death_demo_mode;
        }
        if (Game_IsInGym()) {
            return (GF_COMMAND) { .action = GF_EXIT_TO_TITLE };
        }
        if (g_GameFlow.cmd_death_in_game.action != GF_NOOP) {
            return g_GameFlow.cmd_death_in_game;
        }
        if (g_OverlayFlag == 2) {
            g_OverlayFlag = 1;
            const GF_COMMAND gf_cmd = GF_ShowInventory(INV_DEATH_MODE);
            if (gf_cmd.action != GF_NOOP) {
                return gf_cmd;
            }
        } else {
            g_OverlayFlag = 2;
        }
    }

    if (((g_InputDB.load || g_InputDB.save || g_InputDB.option)
         || g_OverlayFlag <= 0)
        && lara->death_timer == 0 && !lara->extra_anim) {
        if (g_OverlayFlag > 0) {
            if (g_GameFlow.load_save_disabled) {
                g_OverlayFlag = 0;
            } else if (g_Input.save) {
                g_OverlayFlag = -2;
            } else {
                g_OverlayFlag = g_Input.load ? -1 : 0;
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
            if (gf_cmd.action != GF_NOOP) {
                return gf_cmd;
            }
        }
    }

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

    if (!Game_IsInGym() || Gym_IsAssaultTimerActive()) {
        Stats_UpdateTimer();
    }

    return (GF_COMMAND) { .action = GF_NOOP };
}

void Game_Draw(bool draw_overlay)
{
    Interpolation_Interpolate();
    Camera_Apply();
    Room_DrawAllRooms(g_Camera.interp.room_num);
    if (draw_overlay) {
        Overlay_DrawGameInfo();
    }
    SceneCompositor_Flush();
    Game_DrawFade();
}

void Game_ProcessInput(void)
{
    if (GF_GetCurrentLevel()->type == GFL_DEMO) {
        return;
    }

    if (g_InputDB.use_small_medi && Inv_RequestItem(O_SMALL_MEDIPACK_OPTION)) {
        Lara_UseItem(O_SMALL_MEDIPACK_OPTION);
    }
    if (g_InputDB.use_big_medi && Inv_RequestItem(O_LARGE_MEDIPACK_OPTION)) {
        Lara_UseItem(O_LARGE_MEDIPACK_OPTION);
    }

    if (g_GameFlow.load_save_disabled) {
        g_Input.save = 0;
        g_Input.load = 0;
    }

    if (g_InputDB.toggle_ui) {
        UI_ToggleState(&g_Config.ui.enable_game_ui);
    }
}
