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
    if (level_type == GFL_SAVED) {
        // reset start info to the defaults so that we do not do
        // GlobalItemReplace in the inventory initialization routines too early
        ResetStartInfo(level_num);
    } else {
        InitialiseLevelFlags();
    }

    if (!InitialiseLevel(level_num)) {
        return false;
    }

    if (level_type == GFL_SAVED) {
        if (!SaveGame_Load(g_GameInfo.save_slot_to_load, &g_GameInfo)) {
            LOG_ERROR("Failed to load save file!");
            return false;
        }
    }

    return true;
}

int32_t StopGame()
{
    CreateEndInfo(g_CurrentLevel);

    if (g_CurrentLevel == g_GameFlow.last_level_num) {
        g_GameInfo.bonus_flag = GBF_NGPLUS;
    } else {
        CreateStartInfo(g_CurrentLevel + 1);
        ModifyStartInfo(g_CurrentLevel + 1);
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
        return GF_START_GAME | g_GameFlow.first_level_num;
    } else {
        return GF_EXIT_TO_TITLE;
    }
}

int32_t GameLoop(GAMEFLOW_LEVEL_TYPE level_type)
{
    g_OverlayFlag = 1;
    InitialiseCamera();

    Stats_CalculateStats();
    g_GameInfo.stats.max_pickup_count = Stats_GetPickups();
    g_GameInfo.stats.max_kill_count = Stats_GetKillables();
    g_GameInfo.stats.max_secret_count = Stats_GetSecrets();

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
                SaveGame_Save(g_InvExtraData[1], &g_GameInfo);
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

int32_t LevelCompleteSequence(int32_t level_num)
{
    return GF_EXIT_TO_TITLE;
}
