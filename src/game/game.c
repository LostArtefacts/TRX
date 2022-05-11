#include "game/game.h"

#include "config.h"
#include "game/camera.h"
#include "game/control.h"
#include "game/draw.h"
#include "game/gameflow.h"
#include "game/input.h"
#include "game/inv.h"
#include "game/level.h"
#include "game/music.h"
#include "game/savegame.h"
#include "game/settings.h"
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
    g_GameInfo.current_level_type = level_type;

    switch (level_type) {
    case GFL_SAVED:
        // reset current info to the defaults so that we do not do
        // Item_GlobalReplace in the inventory initialization routines too early
        Savegame_InitCurrentInfo();
        if (!Level_Initialise(level_num)) {
            return false;
        }
        if (!Savegame_Load(g_GameInfo.current_save_slot, &g_GameInfo)) {
            LOG_ERROR("Failed to load save file!");
            return false;
        }
        break;

    case GFL_RESTART:
        Savegame_ResetCurrentInfo(level_num);
        if (level_num <= g_GameFlow.first_level_num) {
            Savegame_InitCurrentInfo();
        } else {
            // Use previous level's ending info to start current level.
            Savegame_CarryCurrentInfoToNextLevel(level_num - 1, level_num);
            Savegame_ApplyLogicToCurrentInfo(level_num);
        }
        Level_InitialiseFlags();
        if (!Level_Initialise(level_num)) {
            return false;
        }
        break;

    case GFL_SELECT:
        // reset current info to the defaults so that we do not do
        // Item_GlobalReplace in the inventory initialization routines too early
        Savegame_InitCurrentInfo();
        Savegame_LoadOnlyResumeInfo(g_GameInfo.current_save_slot, &g_GameInfo);
        for (int i = level_num; i < g_GameFlow.level_count; i++) {
            Savegame_ResetCurrentInfo(i);
        }
        if (level_num <= g_GameFlow.first_level_num) {
            Savegame_InitCurrentInfo();
        } else {
            // Use previous level's ending info to start current level.
            Savegame_CarryCurrentInfoToNextLevel(level_num - 1, level_num);
            Savegame_ApplyLogicToCurrentInfo(level_num);
        }
        Level_InitialiseFlags();
        if (!Level_Initialise(level_num)) {
            return false;
        }
        break;

    default:
        Level_InitialiseFlags();
        if (!Level_Initialise(level_num)) {
            return false;
        }
        break;
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
        Savegame_CarryCurrentInfoToNextLevel(
            g_CurrentLevel, g_CurrentLevel + 1);
        Savegame_ApplyLogicToCurrentInfo(g_CurrentLevel + 1);
    }

    g_GameInfo.current[g_CurrentLevel].flags.available = 0;

    if (g_LevelComplete) {
        return GF_LEVEL_COMPLETE | g_CurrentLevel;
    }

    if (!g_InvChosen) {
        return GF_EXIT_TO_TITLE;
    }

    if (g_GameInfo.passport_page == PASSPORT_PAGE_1) {
        return GF_START_SAVED_GAME | g_GameInfo.current_save_slot;
    } else if (
        g_GameInfo.passport_page == PASSPORT_PAGE_1
        && g_GameInfo.passport_mode == PASSPORT_MODE_SELECT_LEVEL) {
        return GF_SELECT_GAME | g_GameInfo.select_level_num;
    } else if (g_GameInfo.passport_page == PASSPORT_PAGE_2) {
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
    Camera_Initialise();

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
                Savegame_Save(g_GameInfo.current_save_slot, &g_GameInfo);
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
