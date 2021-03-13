#include "game/cinema.h"
#include "game/demo.h"
#include "game/game.h"
#include "game/gameflow.h"
#include "game/inv.h"
#include "game/savegame.h"
#include "game/setup.h"
#include "game/text.h"
#include "game/vars.h"
#include "specific/display.h"
#include "specific/file.h"
#include "specific/frontend.h"
#include "specific/init.h"
#include "specific/output.h"
#include "specific/shed.h"
#include "specific/shell.h"
#include "specific/sndpc.h"
#include "config.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>

// T1M: this function was inlined
void S_ReadUserSettings()
{
    FILE *fp = fopen("atiset.dat", "rb");
    if (fp) {
        fread(&OptionMusicVolume, sizeof(int16_t), 1, fp);
        fread(&OptionSoundFXVolume, sizeof(int16_t), 1, fp);
        fread(Layout[1], sizeof(int16_t), 13, fp);
        fread(&AppSettings, sizeof(int32_t), 1, fp);
        fread(&GameHiRes, sizeof(int32_t), 1, fp);
        fread(&GameSizer, sizeof(double), 1, fp);
        if (OptionMusicVolume) {
            S_CDVolume(25 * OptionMusicVolume + 5);
        } else {
            S_CDVolume(0);
        }
        // T1M
        if (OptionSoundFXVolume) {
            adjust_master_volume(6 * OptionSoundFXVolume + 3);
        } else {
            adjust_master_volume(0);
        }
        fclose(fp);
    }
}

// T1M: this function was inlined
void S_WriteUserSettings()
{
    FILE *fp = fopen("atiset.dat", "wb");
    if (fp) {
        fwrite(&OptionMusicVolume, sizeof(int16_t), 1, fp);
        fwrite(&OptionSoundFXVolume, sizeof(int16_t), 1, fp);
        fwrite(Layout[1], sizeof(int16_t), 13, fp);
        fwrite(&AppSettings, sizeof(int32_t), 1, fp);
        fwrite(&GameHiRes, sizeof(int32_t), 1, fp);
        fwrite(&GameSizer, sizeof(double), 1, fp);
        fclose(fp);
    }
}

void GameMain()
{
    SoundIsActive = 1;
    HiRes = 0;
    GameHiRes = 0;
    ScreenSizer = 1.0;
    GameSizer = 1.0;

    S_InitialiseSystem();

    // T1M gameflow support
    if (!GF_LoadScriptFile("Tomb1Main_gameflow.json5")) {
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

    if (!T1MConfig.disable_fmv) { // T1M
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
            // T1M: StartGame(gf_param)
            gf_option = GF_InterpretSequence(gf_param, GFL_NORMAL);
            break;

        // T1M: GF_START_SAVED_GAME does not exist in OG
        case GF_START_SAVED_GAME:
            S_LoadGame(&SaveGame, gf_param);
            gf_option = GF_InterpretSequence(SaveGame.current_level, GFL_SAVED);
            break;

        case GF_START_CINE:
            gf_option = GF_InterpretSequence(gf_param, GFL_CUTSCENE); // T1M
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
            Display_Inventory(1);
            dword_45B940 = 1;

            S_FadeToBlack();
            S_CDStop();

            if (ResetFlag) {
                ResetFlag = 0;
                gf_option = GF_START_DEMO;
            } else if (InventoryChosen == O_PHOTO_OPTION) {
                gf_option = GF_START_GAME | GF.gym_level_num;
            } else if (InventoryChosen == O_PASSPORT_OPTION) {
                if (InventoryExtraData[0] == 0) {
                    gf_option = GF_START_SAVED_GAME | InventoryExtraData[1];
                } else if (InventoryExtraData[0] == 1) {
#ifdef T1M_FEAT_GAMEPLAY
                    SaveGame.bonus_flag = InventoryExtraData[1];
#endif
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
