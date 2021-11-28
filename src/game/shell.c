#include "game/shell.h"

#include "3dsystem/phd_math.h"
#include "config.h"
#include "game/clock.h"
#include "game/demo.h"
#include "game/fmv.h"
#include "game/game.h"
#include "game/gamebuf.h"
#include "game/gameflow.h"
#include "game/input.h"
#include "game/inv.h"
#include "game/music.h"
#include "game/random.h"
#include "game/savegame.h"
#include "game/screen.h"
#include "game/settings.h"
#include "game/setup.h"
#include "game/sound.h"
#include "game/text.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "init.h"
#include "log.h"
#include "memory.h"
#include "specific/s_display.h"
#include "specific/s_frontend.h"
#include "specific/s_hwr.h"
#include "specific/s_input.h"
#include "specific/s_output.h"
#include "specific/s_shell.h"

#include <stdint.h>
#include <stdio.h>
#include <stdio.h>
#include <string.h>

static const char *m_T1MGameflowPath = "cfg/Tomb1Main_gameflow.json5";
static const char *m_T1MGameflowGoldPath = "cfg/Tomb1Main_gameflow_ub.json5";

static void Shell_CalculateWibbleTable();

static void Shell_CalculateWibbleTable()
{
    for (int i = 0; i < WIBBLE_SIZE; i++) {
        PHD_ANGLE angle = (i * PHD_360) / WIBBLE_SIZE;
        g_WibbleTable[i] = phd_sin(angle) * MAX_WIBBLE >> W2V_SHIFT;
        g_ShadeTable[i] = phd_sin(angle) * MAX_SHADE >> W2V_SHIFT;
        g_RandTable[i] = (Random_GetDraw() >> 5) - 0x01FF;
    }
}

void Shell_Main()
{
    g_SoundIsActive = true;

    T1MInit();
    Config_Read();

    const char *gameflow_path = m_T1MGameflowPath;

    char **args;
    int arg_count;
    S_Shell_GetCommandLine(&arg_count, &args);
    for (int i = 0; i < arg_count; i++) {
        if (!strcmp(args[i], "-gold")) {
            gameflow_path = m_T1MGameflowGoldPath;
        }
    }
    for (int i = 0; i < arg_count; i++) {
        Memory_Free(args[i]);
    }
    Memory_Free(args);

    S_Shell_SeedRandom();
    Shell_CalculateWibbleTable();

    Text_Init();
    Clock_Init();
    g_SoundIsActive = Sound_Init();
    Music_Init();
    Input_Init();
    FMV_Init();

    HWR_InitialiseHardware();

    if (!GameFlow_LoadFromFile(gameflow_path)) {
        S_Shell_ExitSystem("MAIN: unable to load script file");
        return;
    }

    InitialiseStartInfo();
    S_FrontEndCheck();
    S_ReadUserSettings();

    Screen_SetResolution(2);
    S_DisplayPicture("data\\eidospc.pcx");
    S_InitialisePolyList();
    S_CopyBufferToScreen();
    S_OutputPolyList();
    S_DumpScreen();
    S_Wait(TICKS_PER_SECOND);

    FMV_Play("fmv\\core.rpl");
    FMV_Play("fmv\\escape.rpl");
    FMV_Play("fmv\\cafe.rpl");

    int32_t gf_option = GF_EXIT_TO_TITLE;

    bool loop_continue = true;
    while (loop_continue) {
        Screen_RestoreResolution();
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
            Text_RemoveAll();
            Screen_SetResolution(2);
            S_DisplayPicture(g_GameFlow.main_menu_background_path);
            g_NoInputCount = 0;
            if (!InitialiseLevel(g_GameFlow.title_level_num, GFL_TITLE)) {
                gf_option = GF_EXIT_GAME;
                break;
            }

            gf_option = Display_Inventory(INV_TITLE_MODE);

            S_FadeToBlack();
            Music_Stop();
            break;

        case GF_EXIT_GAME:
            loop_continue = false;
            break;

        default:
            S_Shell_ExitSystemFmt(
                "MAIN: Unknown request %x %d", gf_direction, gf_param);
            return;
        }
    }

    S_WriteUserSettings();
}
