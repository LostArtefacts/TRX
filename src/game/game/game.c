#include "game/game.h"

#include "config.h"
#include "game/camera.h"
#include "game/effects.h"
#include "game/gameflow.h"
#include "game/input.h"
#include "game/inventory.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/lara/lara_cheat.h"
#include "game/lara/lara_hair.h"
#include "game/level.h"
#include "game/music.h"
#include "game/output.h"
#include "game/overlay.h"
#include "game/savegame.h"
#include "game/shell.h"
#include "game/sound.h"
#include "game/stats.h"
#include "global/const.h"
#include "global/vars.h"
#include "log.h"

#define FRAME_BUFFER(key)                                                      \
    do {                                                                       \
        Game_DrawScene(true);                                                  \
        Output_DumpScreen();                                                   \
        Input_Update();                                                        \
    } while (g_Input.key);

static const int32_t m_AnimationRate = 0x8000;
static int32_t m_FrameCount = 0;

static int32_t Game_Control(int32_t nframes, GAMEFLOW_LEVEL_TYPE level_type);

static int32_t Game_Control(int32_t nframes, GAMEFLOW_LEVEL_TYPE level_type)
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

        if (level_type == GFL_DEMO) {
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
            if (level_type == GFL_DEMO) {
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

        if (((g_Config.enable_buffering ? g_Input.option : g_InputDB.option)
             || g_Input.save || g_Input.load || g_OverlayFlag <= 0)
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
            if (Game_Pause()) {
                return GF_EXIT_TO_TITLE;
            }
        }

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

        m_FrameCount -= 0x10000;
    }

    return GF_NOP;
}

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

    if (g_Config.enable_buffering
        && !(g_GameInfo.status & (GMS_IN_INVENTORY | GMS_IN_PAUSE))) {
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
    g_CurrentLevel = level_num;
    g_GameInfo.current_level_type = level_type;
    g_GameInfo.status = GMS_IN_GAME;

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
        Level_InitialiseFlags();
        if (!Level_Initialise(level_num)) {
            return false;
        }
        break;

    case GFL_SELECT:
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
        Level_InitialiseFlags();
        if (!Level_Initialise(level_num)) {
            return false;
        }
        break;

    case GFL_GYM:
        Savegame_InitCurrentInfo();
        Level_InitialiseFlags();
        if (!Level_Initialise(level_num)) {
            return false;
        }
        break;

    case GFL_BONUS:
        Savegame_CarryCurrentInfoToNextLevel(level_num - 1, level_num);
        Savegame_ApplyLogicToCurrentInfo(level_num);
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
    // https://github.com/LostArtefacts/TR1X/issues/36
    g_Lara.request_gun_type = LGT_UNARMED;

    return true;
}

int32_t Game_Stop(void)
{
    Savegame_PersistGameToCurrentInfo(g_CurrentLevel);

    if (g_CurrentLevel == g_GameFlow.last_level_num) {
        g_Config.profile.new_game_plus_unlock = true;
        Config_Write();
        g_GameInfo.bonus_level_unlock =
            Stats_CheckAllSecretsCollected(GFL_NORMAL);
    }

    if (g_CurrentLevel + 1 < g_GameFlow.level_count) {
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
    } else if (
        g_GameInfo.passport_page == PASSPORT_PAGE_1
        && g_GameInfo.passport_mode == PASSPORT_MODE_STORY_SO_FAR) {
        // page 1: story so far
        return GF_STORY_SO_FAR | g_GameInfo.current_save_slot;
    } else if (g_GameInfo.passport_page == PASSPORT_PAGE_2) {
        return GF_START_GAME
            | (g_InvMode == INV_DEATH_MODE ? g_CurrentLevel
                                           : g_GameFlow.first_level_num);
    } else {
        return GF_EXIT_TO_TITLE;
    }
}

int32_t Game_Loop(GAMEFLOW_LEVEL_TYPE level_type)
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

    bool ask_for_save = g_Config.enable_save_crystals
        && (level_type == GFL_NORMAL || level_type == GFL_BONUS)
        && g_CurrentLevel != g_GameFlow.first_level_num
        && g_CurrentLevel != g_GameFlow.gym_level_num;

    int32_t nframes = 1;
    int32_t ret;
    while (1) {
        ret = Game_Control(nframes, level_type);
        if (ret != GF_NOP) {
            break;
        }
        nframes = Game_ProcessFrame();

        if (ask_for_save) {
            int32_t return_val = Inv_Display(INV_SAVE_CRYSTAL_MODE);
            if (return_val != GF_NOP) {
                Savegame_Save(g_GameInfo.current_save_slot, &g_GameInfo);
                Config_Write();
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
