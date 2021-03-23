#include "specific/shell.h"

#include "args.h"
#include "game/demo.h"
#include "game/game.h"
#include "game/gameflow.h"
#include "game/inv.h"
#include "game/savegame.h"
#include "game/settings.h"
#include "game/setup.h"
#include "game/text.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "specific/display.h"
#include "specific/frontend.h"
#include "specific/init.h"
#include "specific/output.h"
#include "specific/shed.h"
#include "specific/smain.h"
#include "specific/sndpc.h"
#include "util.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void GameMain()
{
    SoundIsActive = 1;
    HiRes = 0;
    GameHiRes = 0;
    ScreenSizer = 1.0;
    GameSizer = 1.0;

    const char *gameflow_path = "cfg/Tomb1Main_gameflow.json5";

    char **args;
    int arg_count;
    get_command_line(&args, &arg_count);
    for (int i = 0; i < arg_count; i++) {
        if (!strcmp(args[i], "-gold")) {
            gameflow_path = "cfg/Tomb1Main_gameflow_ub.json5";
        }
    }

    for (int i = 0; i < arg_count; i++) {
        free(args[i]);
    }
    free(args);

    S_InitialiseSystem();

    if (!GF_LoadScriptFile(gameflow_path)) {
        ShowFatalError("MAIN: unable to load script file");
        return;
    }

    InitialiseStartInfo();
    S_FrontEndCheck();
    S_ReadUserSettings();

    if (IsHardwareRenderer) {
        GameSizer = 1.0;
        dword_45E960 = AppSettings;
    }
    HiRes = 0;
    TempVideoAdjust(2, 1.0);
    S_DisplayPicture("data\\eidospc");
    sub_408E41();
    S_Wait(60);

    if (IsHardwareRenderer) {
        HardwarePrepareFMV();
    }
    WinPlayFMV(FMV_CORE, 1);
    WinPlayFMV(FMV_ESCAPE, 1);
    WinPlayFMV(FMV_INTRO, 1);
    if (!IsHardwareRenderer) {
        HiRes = -1;
    } else {
        HardwareFMVDone();
        if (!IsHardwareRenderer) {
            HiRes = -1;
        }
    }

    GameMemoryPointer = malloc(MALLOC_SIZE);
    if (!GameMemoryPointer) {
        S_ExitSystem("ERROR: Could not allocate enough memory");
        return;
    }

    int32_t gf_option = GF_EXIT_TO_TITLE;

    int8_t loop_continue = 1;
    while (loop_continue) {
        TempVideoRemove();
        int32_t gf_direction = gf_option & ~((1 << 6) - 1);
        int32_t gf_param = gf_option & ((1 << 6) - 1);
        TRACE("%d %d", gf_direction, gf_param);

        switch (gf_direction) {
        case GF_START_GAME:
            gf_option = GF_InterpretSequence(gf_param, GFL_NORMAL);
            break;

        case GF_START_SAVED_GAME:
            S_LoadGame(&SaveGame, gf_param);
            gf_option = GF_InterpretSequence(SaveGame.current_level, GFL_SAVED);
            break;

        case GF_START_CINE:
            gf_option = GF_InterpretSequence(gf_param, GFL_CUTSCENE);
            break;

        case GF_START_DEMO:
            gf_option = StartDemo();
            break;

        case GF_LEVEL_COMPLETE:
            gf_option = LevelCompleteSequence(gf_param);
            break;

        case GF_EXIT_TO_TITLE:
            T_InitPrint();
            TempVideoAdjust(2, 1.0);
            S_DisplayPicture("data\\titleh");
            NoInputCount = 0;
            if (!InitialiseLevel(GF.title_level_num, GFL_TITLE)) {
                gf_option = GF_EXIT_GAME;
                break;
            }
            TitleLoaded = 1;

            dword_45B940 = 0;
            Display_Inventory(INV_TITLE_MODE);
            dword_45B940 = 1;

            S_FadeToBlack();
            S_CDStop();

            if (ResetFlag) {
                ResetFlag = 0;
                gf_option = GF_START_DEMO;
            } else if (InvChosen == O_PHOTO_OPTION) {
                gf_option = GF_START_GAME | GF.gym_level_num;
            } else if (InvChosen == O_PASSPORT_OPTION) {
                if (InvExtraData[0] == 0) {
                    gf_option = GF_START_SAVED_GAME | InvExtraData[1];
                } else if (InvExtraData[0] == 1) {
                    switch (InvExtraData[1]) {
                    case 0:
                        SaveGame.bonus_flag = 0;
                        break;
                    case 1:
                        SaveGame.bonus_flag = GBF_NGPLUS;
                        break;
                    case 2:
                        SaveGame.bonus_flag = GBF_JAPANESE;
                        break;
                    case 3:
                        SaveGame.bonus_flag = GBF_JAPANESE | GBF_NGPLUS;
                        break;
                    }
                    InitialiseStartInfo();
                    gf_option = GF_START_GAME | GF.first_level_num;
                } else {
                    gf_option = GF_EXIT_GAME;
                }
            } else {
                gf_option = GF_EXIT_GAME;
            }
            break;

        case GF_EXIT_GAME:
            loop_continue = 0;
            break;

        default:
            sprintf(
                StringToShow, "MAIN: Unknown request %x %d", gf_direction,
                gf_param);
            S_ExitSystem(StringToShow);
            return;
        }
    }

    S_WriteUserSettings();
}

void T1MInjectSpecificShell()
{
    INJECT(0x00438410, GameMain);
}
