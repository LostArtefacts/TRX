#include "game/shell.h"

#include "3dsystem/phd_math.h"
#include "config.h"
#include "filesystem.h"
#include "game/clock.h"
#include "game/demo.h"
#include "game/fmv.h"
#include "game/game.h"
#include "game/gamebuf.h"
#include "game/gameflow.h"
#include "game/input.h"
#include "game/inv.h"
#include "game/music.h"
#include "game/output.h"
#include "game/random.h"
#include "game/savegame.h"
#include "game/screen.h"
#include "game/settings.h"
#include "game/setup.h"
#include "game/shell.h"
#include "game/sound.h"
#include "game/text.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "init.h"
#include "log.h"
#include "memory.h"
#include "specific/s_input.h"
#include "specific/s_misc.h"
#include "specific/s_shell.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define LEVEL_TITLE_SIZE 25
#define SS_FOLDER_NAME_SIZE 12

static const char *m_T1MGameflowPath = "cfg/Tomb1Main_gameflow.json5";
static const char *m_T1MGameflowGoldPath = "cfg/Tomb1Main_gameflow_ub.json5";

void Shell_Main()
{
    T1MInit();
    Config_Read();

    const char *gameflow_path = m_T1MGameflowPath;

    char **args = NULL;
    int arg_count = 0;
    S_Shell_GetCommandLine(&arg_count, &args);
    for (int i = 0; i < arg_count; i++) {
        if (!strcmp(args[i], "-gold")) {
            gameflow_path = m_T1MGameflowGoldPath;
        }
    }
    for (int i = 0; i < arg_count; i++) {
        Memory_FreePointer(&args[i]);
    }
    Memory_FreePointer(&args);

    S_Shell_SeedRandom();

    if (!Output_Init()) {
        Shell_ExitSystem("Could not initialise video system");
        return;
    }

    Text_Init();
    Clock_Init();
    Sound_Init();
    Music_Init();
    Input_Init();
    FMV_Init();

    if (!GameFlow_LoadFromFile(gameflow_path)) {
        Shell_ExitSystem("MAIN: unable to load script file");
        return;
    }

    InitialiseStartInfo();
    Game_ScanSavedGames();
    Settings_Read();

    Screen_ApplyResolution();

    int32_t gf_option = GF_EXIT_TO_TITLE;
    bool intro_played = false;

    bool loop_continue = true;
    while (loop_continue) {
        int32_t gf_direction = gf_option & ~((1 << 6) - 1);
        int32_t gf_param = gf_option & ((1 << 6) - 1);
        LOG_INFO("%d %d", gf_direction, gf_param);

        switch (gf_direction) {
        case GF_START_GAME:
            gf_option = GameFlow_InterpretSequence(gf_param, GFL_NORMAL);
            break;

        case GF_START_SAVED_GAME:
            S_LoadGame(&g_SaveGame, gf_param);
            gf_option =
                GameFlow_InterpretSequence(g_SaveGame.current_level, GFL_SAVED);
            break;

        case GF_START_CINE:
            gf_option = GameFlow_InterpretSequence(gf_param, GFL_CUTSCENE);
            break;

        case GF_START_DEMO:
            gf_option = StartDemo();
            break;

        case GF_LEVEL_COMPLETE:
            gf_option = LevelCompleteSequence(gf_param);
            break;

        case GF_EXIT_TO_TITLE:
            if (!intro_played) {
                GameFlow_InterpretSequence(
                    g_GameFlow.title_level_num, GFL_NORMAL);
                intro_played = true;
            }

            Text_RemoveAll();
            Output_DisplayPicture(g_GameFlow.main_menu_background_path);
            g_NoInputCount = 0;
            if (!InitialiseLevel(g_GameFlow.title_level_num, GFL_TITLE)) {
                gf_option = GF_EXIT_GAME;
                break;
            }

            gf_option = Display_Inventory(INV_TITLE_MODE);

            Output_FadeToBlack();
            Music_Stop();
            break;

        case GF_EXIT_GAME:
            loop_continue = false;
            break;

        default:
            Shell_ExitSystemFmt(
                "MAIN: Unknown request %x %d", gf_direction, gf_param);
            return;
        }
    }

    Settings_Write();
    S_Shell_Shutdown();
}

void Shell_ExitSystem(const char *message)
{
    S_Shell_Shutdown();
    S_Shell_ShowFatalError(message);
}

void Shell_ExitSystemFmt(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    char message[150];
    vsnprintf(message, 150, fmt, va);
    va_end(va);
    Shell_ExitSystem(message);
}

void Shell_Wait(int nticks)
{
    for (int i = 0; i < nticks; i++) {
        Input_Update();
        if (g_Input.any) {
            break;
        }
        Clock_SyncTicks(1);
    }
    while (g_Input.any) {
        Input_Update();
    }
}

void Shell_ValidateLevelTitle(char *str)
{
    // Strip non alphanumeric chars
    char *ix = str;
    int idx = 0;
    while (*ix != '\0') {
        if ((*ix >= 'A' && *ix <= 'Z') || (*ix >= 'a' && *ix <= 'z')
            || (*ix >= '0' && *ix <= '9') || *ix == '\'' || *ix == '.'
            || *ix == ' ') {
            idx++;
            *ix++;
        } else {
            memmove(&str[idx], &str[idx + 1], strlen(str) - idx);
        }
    }

    // Large level titles write outside compass and passport bounds
    if (strlen(str) > LEVEL_TITLE_SIZE) {
        str[LEVEL_TITLE_SIZE] = '\0';
        return;
    }

    // If title totally invalid, name it based on level number
    if (strlen(str) == 0) {
        char new_title[10]; // 3 digit level num max
        sprintf(new_title, "Level_%d", g_CurrentLevel);
        memmove(str, new_title, strlen(new_title));
        Memory_Free(new_title);
        return;
    }
}

void Shell_GetScreenshotName(char *str, const char *ext)
{
    // Prepare level title for screenshot
    char level_title[LEVEL_TITLE_SIZE];
    sprintf(level_title, "%s", g_GameFlow.levels[g_CurrentLevel].level_title);

    // Replace spaces with underscores
    char *check = level_title;
    bool prev_us = true; // '_' after time field
    int idx = 0;

    // Remove special chars from screenshots that are allowed in titles
    while (*check != '\0') {
        // Replace spaces with a single underscore
        if (*check == ' ') {
            if (prev_us) {
                memmove(
                    &level_title[idx], &level_title[idx + 1],
                    strlen(level_title) - idx);
            } else {
                *check++ = '_';
                idx++;
                prev_us = true;
            }
        } else if (*check == '\'' || *check == '.') {
            memmove(
                &level_title[idx], &level_title[idx + 1],
                strlen(level_title) - idx);
        } else {
            *check++;
            idx++;
            prev_us = false;
        }
    }

    // If title totally invalid, name it based on level number
    // Check again for valid level titles with only ' ', '\'', or '.'
    if (strlen(level_title) == 0) {
        char new_title[10]; // 3 digit level num max
        sprintf(new_title, "Level_%d", g_CurrentLevel);
        memmove(level_title, new_title, strlen(new_title));
        Memory_Free(new_title);
        prev_us = false;
    }

    // Strip trailing underscores
    if (prev_us) {
        *check--;
        idx--;
        memmove(
            &level_title[idx], &level_title[idx + 1], strlen(level_title) - 1);
        prev_us = false;
    }

    // Get timestamp
    char date_time[20];
    Clock_GetDateTime(date_time);

    // Full screenshot name
    sprintf(str, "%s_%s.%s", date_time, level_title, ext);
}

bool Shell_MakeScreenshot()
{
    const char *ext;
    switch (g_Config.screenshot_format) {
    case SCREENSHOT_FORMAT_JPEG:
        ext = "jpg";
        break;
    case SCREENSHOT_FORMAT_PNG:
        ext = "png";
        break;
    }

    // Screenshot name
    char ss_name[LEVEL_TITLE_SIZE + 25]; // 50
    Shell_GetScreenshotName(ss_name, ext);

    // Screenshot folder
    char ss_folder[SS_FOLDER_NAME_SIZE];
    sprintf(ss_folder, "screenshots");
    File_CreateDirectory(ss_folder);

    // Screenshot folder/name path
    char path[LEVEL_TITLE_SIZE + SS_FOLDER_NAME_SIZE + 63]; // 100
    sprintf(path, "%s/%s", ss_folder, ss_name);

    char *full_path = NULL;
    File_GetFullPath(path, &full_path);
    if (!File_Exists(full_path)) {
        bool result = Output_MakeScreenshot(full_path);
        Memory_Free(full_path);
        full_path = NULL;
        return result;
    }

    return false;
}
