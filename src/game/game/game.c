#include "game/game.h"

#include "config.h"
#include "game/camera.h"
#include "game/clock.h"
#include "game/gameflow.h"
#include "game/input.h"
#include "game/inventory.h"
#include "game/lara.h"
#include "game/level.h"
#include "game/music.h"
#include "game/output.h"
#include "game/phase/phase.h"
#include "game/savegame.h"
#include "game/sound.h"
#include "game/stats.h"
#include "global/vars.h"
#include "log.h"

#define FRAME_BUFFER(key)                                                      \
    do {                                                                       \
        Game_DrawScene(true);                                                  \
        Input_Update();                                                        \
        Output_DumpScreen();                                                   \
        int ticks = Clock_SyncTicks();                                         \
        Output_AnimateFades(ticks);                                            \
    } while (g_Input.key);

void Game_ProcessInput(void)
{
    if (g_Config.enable_numeric_keys) {
        if (g_InputDB.equip_pistols && Inv_RequestItem(O_GUN_ITEM)) {
            g_Lara.request_gun_type = LGT_PISTOLS;
        } else if (g_InputDB.equip_shotgun && Inv_RequestItem(O_SHOTGUN_ITEM)) {
            g_Lara.request_gun_type = LGT_SHOTGUN;
        } else if (g_InputDB.equip_magnums && Inv_RequestItem(O_MAGNUM_ITEM)) {
            g_Lara.request_gun_type = LGT_MAGNUMS;
        } else if (g_InputDB.equip_uzis && Inv_RequestItem(O_UZI_ITEM)) {
            g_Lara.request_gun_type = LGT_UZIS;
        }
    }

    if (g_InputDB.use_small_medi && Inv_RequestItem(O_MEDI_OPTION)) {
        Lara_UseItem(O_MEDI_OPTION);
    } else if (g_InputDB.use_big_medi && Inv_RequestItem(O_BIGMEDI_OPTION)) {
        Lara_UseItem(O_BIGMEDI_OPTION);
    }

    if (g_Config.enable_buffering && Phase_Get() == PHASE_GAME) {
        if (g_Input.toggle_bilinear_filter) {
            FRAME_BUFFER(toggle_bilinear_filter);
        } else if (g_Input.toggle_perspective_filter) {
            FRAME_BUFFER(toggle_perspective_filter);
        } else if (g_Input.toggle_fps_counter) {
            FRAME_BUFFER(toggle_fps_counter);
        }
    }
}

bool Game_Start(int32_t level_num, GAMEFLOW_LEVEL_TYPE level_type)
{
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
        if (level_num <= g_GameFlow.first_level_num) {
            Savegame_InitCurrentInfo();
        } else {
            Savegame_ResetCurrentInfo(level_num);
            // Use previous level's ending info to start current level.
            Savegame_CarryCurrentInfoToNextLevel(level_num - 1, level_num);
            Savegame_ApplyLogicToCurrentInfo(level_num);
        }
        if (!Level_Initialise(level_num)) {
            return false;
        }
        break;

    case GFL_SELECT:
        if (g_GameInfo.current_save_slot != -1) {
            // select level feature
            Savegame_InitCurrentInfo();
            if (level_num > g_GameFlow.first_level_num) {
                Savegame_LoadOnlyResumeInfo(
                    g_GameInfo.current_save_slot, &g_GameInfo);
                for (int i = level_num; i < g_GameFlow.level_count; i++) {
                    Savegame_ResetCurrentInfo(i);
                }
                // Use previous level's ending info to start current level.
                Savegame_CarryCurrentInfoToNextLevel(level_num - 1, level_num);
                Savegame_ApplyLogicToCurrentInfo(level_num);
            }
        } else if (g_CurrentLevel == g_GameFlow.title_level_num) {
            // console /play level feature
            Savegame_InitCurrentInfo();
            for (int i = g_GameFlow.first_level_num + 1; i <= level_num; i++) {
                Savegame_CarryCurrentInfoToNextLevel(i - 1, i);
                Savegame_ApplyLogicToCurrentInfo(i);
            }
        }
        if (!Level_Initialise(level_num)) {
            return false;
        }
        break;

    case GFL_GYM:
        Savegame_ResetCurrentInfo(level_num);
        Savegame_ApplyLogicToCurrentInfo(level_num);
        if (!Level_Initialise(level_num)) {
            return false;
        }
        break;

    case GFL_BONUS:
        Savegame_CarryCurrentInfoToNextLevel(level_num - 1, level_num);
        Savegame_ApplyLogicToCurrentInfo(level_num);
        if (!Level_Initialise(level_num)) {
            return false;
        }
        break;

    default:
        if (!Level_Initialise(level_num)) {
            return false;
        }
        break;
    }

    // LaraGun() expects request_gun_type to be set only when it
    // really is needed, not at all times.
    // https://github.com/LostArtefacts/TR1X/issues/36
    g_Lara.request_gun_type = LGT_UNARMED;

    g_OverlayFlag = 1;
    Camera_Initialise();

    Stats_CalculateStats();
    g_GameInfo.current[g_CurrentLevel].stats.max_pickup_count =
        Stats_GetPickups();
    g_GameInfo.current[g_CurrentLevel].stats.max_kill_count =
        Stats_GetKillables();
    g_GameInfo.current[g_CurrentLevel].stats.max_secret_count =
        Stats_GetSecrets();

    g_GameInfo.ask_for_save = g_Config.enable_save_crystals
        && (level_type == GFL_NORMAL || level_type == GFL_BONUS)
        && g_CurrentLevel != g_GameFlow.first_level_num
        && g_CurrentLevel != g_GameFlow.gym_level_num;

    return true;
}

GAMEFLOW_OPTION Game_Stop(void)
{
    Sound_StopAllSamples();
    Music_Stop();
    Savegame_PersistGameToCurrentInfo(g_CurrentLevel);

    if (g_CurrentLevel == g_GameFlow.last_level_num) {
        g_Config.profile.new_game_plus_unlock = true;
        Config_Write();
        g_GameInfo.bonus_level_unlock =
            Stats_CheckAllSecretsCollected(GFL_NORMAL);
    }

    // play specific level
    if (g_LevelComplete && g_GameInfo.select_level_num != -1) {
        if (g_CurrentLevel != -1) {
            Savegame_CarryCurrentInfoToNextLevel(
                g_CurrentLevel, g_GameInfo.select_level_num);
        }
        return GF_SELECT_GAME | g_GameInfo.select_level_num;
    }

    // carry info to the next level
    if (g_CurrentLevel + 1 < g_GameFlow.level_count) {
        // TODO: this should be moved to GFS_EXIT_TO_LEVEL handler, probably
        Savegame_CarryCurrentInfoToNextLevel(
            g_CurrentLevel, g_CurrentLevel + 1);
        Savegame_ApplyLogicToCurrentInfo(g_CurrentLevel + 1);
    }

    // normal level completion
    if (g_LevelComplete) {
        // TODO: why is this made unavailable?
        g_GameInfo.current[g_CurrentLevel].flags.available = 0;
        return GF_LEVEL_COMPLETE | g_GameInfo.select_level_num;
    }

    if (!g_InvChosen) {
        return GF_EXIT_TO_TITLE;
    }

    if (g_GameInfo.passport_selection == PASSPORT_MODE_LOAD_GAME) {
        return GF_START_SAVED_GAME | g_GameInfo.current_save_slot;
    } else if (g_GameInfo.passport_selection == PASSPORT_MODE_SELECT_LEVEL) {
        return GF_SELECT_GAME | g_GameInfo.select_level_num;
    } else if (g_GameInfo.passport_selection == PASSPORT_MODE_STORY_SO_FAR) {
        return GF_STORY_SO_FAR | g_GameInfo.current_save_slot;
    } else if (g_GameInfo.passport_selection == PASSPORT_MODE_RESTART) {
        return GF_RESTART_GAME | g_CurrentLevel;
    } else if (g_GameInfo.passport_selection == PASSPORT_MODE_NEW_GAME) {
        Savegame_InitCurrentInfo();
        return GF_START_GAME | g_GameFlow.first_level_num;
    } else {
        return GF_EXIT_TO_TITLE;
    }
}
