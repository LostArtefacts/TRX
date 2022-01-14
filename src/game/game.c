#include "game/game.h"

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

    if (!InitialiseLevel(level_num, level_type)) {
        g_CurrentLevel = 0;
        return GF_EXIT_TO_TITLE;
    }

    return GF_NOP;
}

int32_t StopGame()
{
    if (g_LevelComplete) {
        S_FadeInInventory(1);
        return GF_LEVEL_COMPLETE | g_CurrentLevel;
    }

    Output_FadeToBlack();
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

int32_t GameLoop(int32_t demo_mode)
{
    g_NoInputCount = 0;
    g_ResetFlag = false;
    g_OverlayFlag = 1;
    InitialiseCamera();

    int32_t nframes = 1;
    int32_t ret;
    while (1) {
        ret = ControlPhase(nframes, demo_mode);
        if (ret != GF_NOP) {
            break;
        }
        nframes = DrawPhaseGame();
    }

    Sound_StopAllSamples();
    Music_Stop();
    Music_SetVolume(g_Config.music_volume);

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

    Screen_ApplyResolution();
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

    // wait till action key release
    while (g_Input.select || g_Input.deselect) {
        Input_Update();
        Output_InitialisePolyList();
        Output_CopyBufferToScreen();
        Input_Update();
        Text_Draw();
        Output_DumpScreen();
    }

    // wait till action or escape key press
    while (!g_Input.select && !g_Input.deselect) {
        if (g_ResetFlag) {
            break;
        }
        Output_InitialisePolyList();
        Output_CopyBufferToScreen();
        Input_Update();
        Text_Draw();
        Output_DumpScreen();
    }

    // wait till escape key release
    while (g_Input.deselect) {
        Output_InitialisePolyList();
        Output_CopyBufferToScreen();
        Input_Update();
        Text_Draw();
        Output_DumpScreen();
    }

    if (level_num == g_GameFlow.last_level_num) {
        g_GameInfo.bonus_flag = GBF_NGPLUS;
    } else {
        CreateStartInfo(level_num + 1);
        ModifyStartInfo(level_num + 1);
    }

    g_GameInfo.start[g_CurrentLevel].flags.available = 0;
    Output_FadeToBlack();
    Screen_ApplyResolution();
}
