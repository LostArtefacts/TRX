#include "game/game.h"

#include "game/inv.h"
#include "game/settings.h"
#include "config.h"
#include "game/camera.h"
#include "game/control.h"
#include "game/draw.h"
#include "game/gameflow.h"
#include "game/input.h"
#include "game/music.h"
#include "game/savegame.h"
#include "game/setup.h"
#include "game/shell.h"
#include "game/sound.h"
#include "game/stats.h"
#include "global/const.h"
#include "global/vars.h"
#include "log.h"
#include "specific/s_misc.h"

#include <stdio.h>

bool StartGame(int32_t level_num, GAMEFLOW_LEVEL_TYPE level_type)
{
    g_CurrentLevel = level_num;
    g_GameInfo.level_type = level_type;
    if (level_type == GFL_SAVED) {
        // reset start info to the defaults so that we do not do
        // GlobalItemReplace in the inventory initialization routines too early
        Savegame_ResetStartInfo(level_num);
    } else {
        InitialiseLevelFlags();
    }

    if (!InitialiseLevel(level_num)) {
        return false;
    }

    if (level_type == GFL_SAVED) {
        if (!Savegame_Load(g_GameInfo.current_save_slot, &g_GameInfo)) {
            LOG_ERROR("Failed to load save file!");
            return false;
        }
    }

    // LaraGun() expects request_gun_type to be set only when it
    // really is needed, not at all times.
    // https://github.com/rr-/Tomb1Main/issues/36
    g_Lara.request_gun_type = LGT_UNARMED;

    return true;
}

int32_t StopGame(void)
{
    Savegame_PersistGameToCurrentInfo(g_CurrentLevel);

    if (g_CurrentLevel == g_GameFlow.last_level_num) {
        g_GameInfo.bonus_flag = GBF_NGPLUS;
    } else {
        Savegame_CarryCurrentInfoToStartInfo(
            g_CurrentLevel, g_CurrentLevel + 1);
        Savegame_ApplyLogicToStartInfo(g_CurrentLevel + 1);
    }

    g_GameInfo.start[g_CurrentLevel].flags.available = 0;

    if (g_LevelComplete) {
        return GF_LEVEL_COMPLETE | g_CurrentLevel;
    }

    if (!g_InvChosen) {
        return GF_EXIT_TO_TITLE;
    }

    if (g_InvExtraData[0] == 0) {
        return GF_START_SAVED_GAME | g_InvExtraData[1];
    } else if (g_InvExtraData[0] == 1) {
        return GF_START_GAME
            | (g_InvMode == INV_DEATH_MODE ? g_CurrentLevel
                                           : g_GameFlow.first_level_num);
    } else {
        return GF_EXIT_TO_TITLE;
    }
}

int32_t GameLoop(GAMEFLOW_LEVEL_TYPE level_type)
{
    g_OverlayFlag = 1;
    InitialiseCamera();

    Stats_CalculateStats();
    g_GameInfo.current[g_CurrentLevel].stats.max_pickup_count =
        Stats_GetPickups();
    g_GameInfo.current[g_CurrentLevel].stats.max_kill_count =
        Stats_GetKillables();
    g_GameInfo.current[g_CurrentLevel].stats.max_secret_count =
        Stats_GetSecrets();

    bool ask_for_save = g_GameFlow.enable_save_crystals
        && level_type == GFL_NORMAL
        && g_CurrentLevel != g_GameFlow.first_level_num
        && g_CurrentLevel != g_GameFlow.gym_level_num;

    int32_t nframes = 1;
    int32_t ret;
    while (1) {
        ret = ControlPhase(nframes, level_type);
        if (ret != GF_NOP) {
            break;
        }
        nframes = Draw_ProcessFrame();

        if (ask_for_save) {
            int32_t return_val = Display_Inventory(INV_SAVE_CRYSTAL_MODE);
            if (return_val != GF_NOP) {
                Savegame_Save(g_InvExtraData[1], &g_GameInfo);
                Settings_Write();
            }
            ask_for_save = false;
        }
    }

    Sound_StopAllSamples();
    Music_Stop();

    if (ret == GF_NOP_BREAK) {
        return GF_NOP;
    }

    return ret;
}
