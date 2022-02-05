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
    if (level_type != GFL_SAVED) {
        InitialiseLevelFlags();
    }

    if (!InitialiseLevel(level_num)) {
        return false;
    }

    if (level_type == GFL_SAVED) {
        if (!SaveGame_ApplySaveBuffer(&g_GameInfo)) {
            LOG_ERROR("Failed to load save file!");
            return false;
        }
    }

    return true;
}

int32_t StopGame()
{
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
    g_NoInputCount = 0;
    g_ResetFlag = false;
    g_OverlayFlag = 1;
    InitialiseCamera();

    Stats_CalculateStats();
    g_GameFlow.levels[g_CurrentLevel].pickups = Stats_GetPickups();
    g_GameFlow.levels[g_CurrentLevel].kills = Stats_GetKillables();
    g_GameFlow.levels[g_CurrentLevel].secrets = Stats_GetSecrets();

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
                SaveGame_SaveToFile(&g_GameInfo, g_InvExtraData[1]);
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
