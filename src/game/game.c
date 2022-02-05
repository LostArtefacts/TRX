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
#include "game/output.h"
#include "game/savegame.h"
#include "game/screen.h"
#include "game/setup.h"
#include "game/shell.h"
#include "game/sound.h"
#include "game/text.h"
#include "global/const.h"
#include "global/vars.h"
#include "log.h"
#include "specific/s_misc.h"

#include <stdio.h>

int32_t StartGame(int32_t level_num, GAMEFLOW_LEVEL_TYPE level_type)
{
    g_CurrentLevel = level_num;
    if (level_type != GFL_SAVED) {
        InitialiseLevelFlags();
    }

    if (!InitialiseLevel(level_num)) {
        g_CurrentLevel = 0;
        return GF_EXIT_TO_TITLE;
    }

    if (level_type == GFL_SAVED) {
        SaveGame_ApplySaveBuffer(&g_GameInfo);
    }

    return GF_NOP;
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

void LevelStats(int32_t level_num)
{
    char string[100];
    char time_str[100];
    TEXTSTRING *txt;

    Text_RemoveAll();

    // heading
    sprintf(string, "%s", g_GameFlow.levels[level_num].level_title);
    txt = Text_Create(0, -50, string);
    Text_CentreH(txt, 1);
    Text_CentreV(txt, 1);

    // time taken
    int32_t seconds = g_GameInfo.timer / 30;
    int32_t hours = seconds / 3600;
    int32_t minutes = (seconds / 60) % 60;
    seconds %= 60;
    if (hours) {
        sprintf(
            time_str, "%d:%d%d:%d%d", hours, minutes / 10, minutes % 10,
            seconds / 10, seconds % 10);
    } else {
        sprintf(time_str, "%d:%d%d", minutes, seconds / 10, seconds % 10);
    }
    sprintf(string, g_GameFlow.strings[GS_STATS_TIME_TAKEN_FMT], time_str);
    txt = Text_Create(0, 70, string);
    Text_CentreH(txt, 1);
    Text_CentreV(txt, 1);

    // secrets
    int32_t secrets_taken = 0;
    int32_t secrets_total = MAX_SECRETS;
    do {
        if (g_GameInfo.secrets & 1) {
            secrets_taken++;
        }
        g_GameInfo.secrets >>= 1;
        secrets_total--;
    } while (secrets_total);
    sprintf(
        string, g_GameFlow.strings[GS_STATS_SECRETS_FMT], secrets_taken,
        g_GameFlow.levels[level_num].secrets);
    txt = Text_Create(0, 40, string);
    Text_CentreH(txt, 1);
    Text_CentreV(txt, 1);

    // pickups
    sprintf(
        string, g_GameFlow.strings[GS_STATS_PICKUPS_FMT], g_GameInfo.pickups);
    txt = Text_Create(0, 10, string);
    Text_CentreH(txt, 1);
    Text_CentreV(txt, 1);

    // kills
    sprintf(string, g_GameFlow.strings[GS_STATS_KILLS_FMT], g_GameInfo.kills);
    txt = Text_Create(0, -20, string);
    Text_CentreH(txt, 1);
    Text_CentreV(txt, 1);

    Output_FadeToSemiBlack(true);
    // wait till a skip key is pressed
    do {
        if (g_ResetFlag) {
            break;
        }
        Output_InitialisePolyList();
        Draw_DrawScene(false);
        Input_Update();
        Text_Draw();
        Output_DumpScreen();
    } while (!g_InputDB.select && !g_InputDB.deselect);

    Output_FadeToBlack(false);
    Text_RemoveAll();

    // finish fading
    while (Output_FadeIsAnimating()) {
        Output_InitialisePolyList();
        Draw_DrawScene(false);
        Output_DumpScreen();
    }

    Output_FadeReset();

    if (level_num == g_GameFlow.last_level_num) {
        g_GameInfo.bonus_flag = GBF_NGPLUS;
    } else {
        CreateStartInfo(level_num + 1);
        ModifyStartInfo(level_num + 1);
    }

    g_GameInfo.start[g_CurrentLevel].flags.available = 0;
    Screen_ApplyResolution();
}
