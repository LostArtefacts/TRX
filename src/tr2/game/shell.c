#include "game/shell.h"

#include "config.h"
#include "decomp/decomp.h"
#include "game/console/common.h"
#include "game/demo.h"
#include "game/game_string.h"
#include "game/gameflow.h"
#include "game/gameflow/reader.h"
#include "game/input.h"
#include "game/music.h"
#include "game/sound.h"
#include "game/text.h"
#include "global/funcs.h"
#include "global/vars.h"

#include <libtrx/enum_map.h>
#include <libtrx/game/ui/common.h>
#include <libtrx/memory.h>

#include <stdarg.h>
#include <stdio.h>

// TODO: refactor the hell out of me
BOOL __cdecl Shell_Main(void)
{
    g_HiRes = 0;
    g_ScreenSizer = 0;
    g_GameSizer = 1.0;
    g_GameSizerCopy = 1.0;

    GameString_Init();
    EnumMap_Init();
    Config_Init();
    Text_Init();
    UI_Init();
    Console_Init();

    Config_Read();
    if (!S_InitialiseSystem()) {
        return false;
    }

    if (!GF_LoadScriptFile("data\\tombPC.dat")) {
        Shell_ExitSystem("GameMain: could not load script file");
        return false;
    }

    if (!GF_N_Load("cfg/TR2X_gameflow.json5")) {
        Shell_ExitSystem("GameMain: could not load new script file");
        return false;
    }

    InitialiseStartInfo();
    S_FrontEndCheck();
    S_LoadSettings();

    g_HiRes = -1;
    g_GameMemoryPtr = GlobalAlloc(0, 0x380000u);

    if (!g_GameMemoryPtr) {
        strcpy(g_ErrorMessage, "GameMain: could not allocate malloc_buffer");
        return false;
    }

    g_HiRes = 0;
    TempVideoAdjust(1, 1.0);
    Input_Update();

    g_IsVidModeLock = 1;
    S_DisplayPicture("data\\legal.pcx", 0);
    S_InitialisePolyList(0);
    S_CopyBufferToScreen();
    S_OutputPolyList();
    S_DumpScreen();
    FadeToPal(30, g_GamePalette8);
    S_Wait(180, 1);
    S_FadeToBlack();
    S_DontDisplayPicture();
    g_IsVidModeLock = 0;

    const bool is_frontend_fail = GF_DoFrontendSequence();
    if (g_IsGameToExit) {
        Config_Write();
        return true;
    }

    if (is_frontend_fail) {
        strcpy(g_ErrorMessage, "GameMain: failed in GF_DoFrontendSequence()");
        return false;
    }

    S_FadeToBlack();
    int16_t gf_option = g_GameFlow.first_option;
    g_NoInputCounter = 0;

    bool is_loop_continued = true;
    while (is_loop_continued) {
        const int16_t gf_dir = gf_option & 0xFF00;
        const int16_t gf_param = gf_option & 0x00FF;

        switch (gf_dir) {
        case GFD_START_GAME:
            if (g_GameFlow.single_level >= 0) {
                gf_option =
                    GF_DoLevelSequence(g_GameFlow.single_level, GFL_NORMAL);
            } else {
                if (gf_param > g_GameFlow.num_levels) {
                    sprintf(
                        g_ErrorMessage,
                        "GameMain: STARTGAME with invalid level number (%d)",
                        gf_param);
                    return false;
                }
                gf_option = GF_DoLevelSequence(gf_param, GFL_NORMAL);
            }
            break;

        case GFD_START_SAVED_GAME:
            S_LoadGame(&g_SaveGame, sizeof(SAVEGAME_INFO), gf_param);
            if (g_SaveGame.current_level > g_GameFlow.num_levels) {
                sprintf(
                    g_ErrorMessage,
                    "GameMain: STARTSAVEDGAME with invalid level number (%d)",
                    g_SaveGame.current_level);
                return false;
            }
            gf_option = GF_DoLevelSequence(g_SaveGame.current_level, GFL_SAVED);
            break;

        case GFD_START_CINE:
            Game_Cutscene_Start(gf_param);
            gf_option = GFD_EXIT_TO_TITLE;
            break;

        case GFD_START_DEMO:
            gf_option = Demo_Control(-1);
            break;

        case GFD_LEVEL_COMPLETE:
            gf_option = LevelCompleteSequence();
            break;

        case GFD_EXIT_TO_TITLE:
        case GFD_EXIT_TO_OPTION:
            if (g_GameFlow.title_disabled) {
                if (g_GameFlow.title_replace < 0
                    || g_GameFlow.title_replace == GFD_EXIT_TO_TITLE) {
                    strcpy(
                        g_ErrorMessage,
                        "GameMain Failed: Title disabled & no replacement");
                    return false;
                }
                gf_option = g_GameFlow.title_replace;
            } else {
                gf_option = TitleSequence();
                g_GF_StartGame = 1;
            }
            break;

        default:
            is_loop_continued = false;
            break;
        }
    }

    S_SaveSettings();
    GameBuf_Shutdown();
    EnumMap_Shutdown();
    GameString_Shutdown();
    return true;
}

void __cdecl Shell_Cleanup(void)
{
    Music_Shutdown();
}

void __cdecl Shell_ExitSystem(const char *message)
{
    GameBuf_Shutdown();
    strcpy(g_ErrorMessage, message);
    Shell_Shutdown();
    Shell_Cleanup();
    exit(1);
}

void __cdecl Shell_ExitSystemFmt(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    int32_t size = vsnprintf(NULL, 0, fmt, va) + 1;
    char *message = Memory_Alloc(size);
    va_end(va);

    va_start(va, fmt);
    vsnprintf(message, size, fmt, va);
    va_end(va);

    Shell_ExitSystem(message);

    Memory_FreePointer(&message);
}
