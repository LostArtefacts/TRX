#include "game/shell.h"

#include "config.h"
#include "filesystem.h"
#include "game/clock.h"
#include "game/fmv.h"
#include "game/game.h"
#include "game/gamebuf.h"
#include "game/gameflow.h"
#include "game/input.h"
#include "game/inventory.h"
#include "game/level.h"
#include "game/music.h"
#include "game/option.h"
#include "game/output.h"
#include "game/savegame.h"
#include "game/screen.h"
#include "game/sound.h"
#include "game/text.h"
#include "global/types.h"
#include "global/vars.h"
#include "log.h"
#include "memory.h"
#include "specific/s_shell.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SCREENSHOTS_DIR "screenshots"
#define LEVEL_TITLE_SIZE 25
#define TIMESTAMP_SIZE 20

static const char *m_T1MGameflowPath = "cfg/Tomb1Main_gameflow.json5";
static const char *m_T1MGameflowGoldPath = "cfg/Tomb1Main_gameflow_ub.json5";

static char *Shell_GetScreenshotName(void);

static char *Shell_GetScreenshotName(void)
{
    // Get level title of unknown length
    char level_title[100];
    snprintf(
        level_title, LEVEL_TITLE_SIZE, "%s",
        g_GameFlow.levels[g_CurrentLevel].level_title);

    // Trim level titles that are too long
    if (strlen(level_title) >= LEVEL_TITLE_SIZE) {
        level_title[LEVEL_TITLE_SIZE] = '\0';
    }

    // Prepare level title for screenshot
    char *check = level_title;
    bool prev_us = true; // '_' after timestamp before title
    int idx = 0;

    while (*check != '\0') {
        if (*check == ' ') {
            // Replace spaces with a single underscore
            if (prev_us) {
                memmove(
                    &level_title[idx], &level_title[idx + 1],
                    strlen(level_title) - idx);
            } else {
                *check++ = '_';
                idx++;
                prev_us = true;
            }
        } else if (((*check < 'A' || *check > 'Z')
                    && (*check < 'a' || *check > 'z')
                    && (*check < '0' || *check > '9'))) {
            // Strip non alphanumeric chars
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
    if (strlen(level_title) == 0) {
        sprintf(level_title, "Level_%d", g_CurrentLevel);
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
    char date_time[TIMESTAMP_SIZE];
    Clock_GetDateTime(date_time);

    // Full screenshot name
    size_t out_size = snprintf(NULL, 0, "%s_%s", date_time, level_title) + 1;
    char *out = Memory_Alloc(out_size);
    snprintf(out, out_size, "%s_%s", date_time, level_title) + 1;
    return out;
}

void Shell_Init(const char *gameflow_path)
{
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

    Option_Init();
    Savegame_InitCurrentInfo();
    Savegame_ScanSavedGames();
    Savegame_HighlightNewestSlot();
    Config_Init();

    Screen_ApplyResolution();
}

void Shell_Shutdown(void)
{
    while (g_Input.select) {
        Input_Update();
    }
    GameFlow_Shutdown();
    GameBuf_Shutdown();
    Output_Shutdown();
    Input_Shutdown();
    Sound_Shutdown();
    Music_Shutdown();
    Savegame_Shutdown();
    Option_Shutdown();
}

void Shell_Main(void)
{
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

    Shell_Init(gameflow_path);

    int32_t gf_option = GF_EXIT_TO_TITLE;
    bool intro_played = false;

    bool loop_continue = true;
    while (loop_continue) {
        int32_t gf_direction = gf_option & ~((1 << 6) - 1);
        int32_t gf_param = gf_option & ((1 << 6) - 1);
        LOG_INFO("%d %d", gf_direction, gf_param);

        switch (gf_direction) {
        case GF_START_GYM:
            gf_option = GameFlow_InterpretSequence(gf_param, GFL_GYM);
            break;

        case GF_START_GAME:
            gf_option = GameFlow_InterpretSequence(gf_param, GFL_NORMAL);
            break;

        case GF_START_SAVED_GAME: {
            int16_t level_num = Savegame_GetLevelNumber(gf_param);
            if (level_num < 0) {
                LOG_ERROR("Corrupt save file!");
                gf_option = GF_EXIT_TO_TITLE;
            } else {
                g_GameInfo.current_save_slot = gf_param;
                gf_option = GameFlow_InterpretSequence(level_num, GFL_SAVED);
            }
            break;
        }

        case GF_RESTART_GAME: {
            gf_option = GameFlow_InterpretSequence(gf_param, GFL_RESTART);
            break;
        }

        case GF_SELECT_GAME: {
            gf_option = GameFlow_InterpretSequence(gf_param, GFL_SELECT);
            break;
        }

        case GF_STORY_SO_FAR: {
            gf_option = Savegame_PlayAvailableStory(gf_param);
            break;
        }

        case GF_START_CINE:
            gf_option = GameFlow_InterpretSequence(gf_param, GFL_CUTSCENE);
            break;

        case GF_START_DEMO:
            Game_Demo();
            gf_option = GF_EXIT_TO_TITLE;
            break;

        case GF_LEVEL_COMPLETE:
            gf_option = GF_EXIT_TO_TITLE;
            break;

        case GF_EXIT_TO_TITLE:
            g_GameInfo.current_save_slot = -1;
            if (!intro_played) {
                GameFlow_InterpretSequence(
                    g_GameFlow.title_level_num, GFL_NORMAL);
                intro_played = true;
            }

            Text_RemoveAll();
            Output_DisplayPicture(g_GameFlow.main_menu_background_path);
            if (!Level_Initialise(g_GameFlow.title_level_num)) {
                gf_option = GF_EXIT_GAME;
                break;
            }

            gf_option = Inv_Display(INV_TITLE_MODE);
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

    Config_Write();
    Shell_Shutdown();
}

void Shell_ExitSystem(const char *message)
{
    Shell_Shutdown();
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
        if (g_InputDB.any) {
            break;
        }
        Clock_SyncTicks(1);
    }
}

bool Shell_MakeScreenshot(void)
{
    File_CreateDirectory(SCREENSHOTS_DIR);

    char *filename = Shell_GetScreenshotName();

    const char *ext;
    switch (g_Config.screenshot_format) {
    case SCREENSHOT_FORMAT_JPEG:
        ext = "jpg";
        break;
    case SCREENSHOT_FORMAT_PNG:
        ext = "png";
        break;
    default:
        ext = "jpg";
        break;
    }

    bool result = false;
    char *full_path = Memory_Alloc(
        strlen(SCREENSHOTS_DIR) + strlen(filename) + strlen(ext) + 6);
    sprintf(full_path, "%s/%s.%s", SCREENSHOTS_DIR, filename, ext);
    if (!File_Exists(full_path)) {
        result = Output_MakeScreenshot(full_path);
    } else {
        // name already exists, so add a number to name
        for (int i = 2; i < 100; i++) {
            sprintf(
                full_path, "%s/%s_%d.%s", SCREENSHOTS_DIR, filename, i, ext);
            if (!File_Exists(full_path)) {
                result = Output_MakeScreenshot(full_path);
                break;
            }
        }
    }

    Memory_FreePointer(&filename);
    Memory_FreePointer(&full_path);
    return result;
}
