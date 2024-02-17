#include "game/phase/phase_stats.h"

#include "config.h"
#include "game/game.h"
#include "game/gameflow.h"
#include "game/input.h"
#include "game/output.h"
#include "game/overlay.h"
#include "game/phase/phase.h"
#include "game/shell.h"
#include "game/text.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define MAX_TEXTSTRINGS 10

typedef enum STATS_STATE {
    STATS_STATE_FADE_IN,
    STATS_STATE_DISPLAY,
    STATS_STATE_FADE_OUT,
} STATS_STATE;

static GAME_STATUS m_OldGameStatus;
static STATS_STATE m_State = STATS_STATE_DISPLAY;
static TEXTSTRING *m_Texts[MAX_TEXTSTRINGS] = { 0 };

static void Phase_Stats_CreateTexts(int32_t level_num);
static void Phase_Stats_Start(void *arg);
static void Phase_Stats_End(void);
static GAMEFLOW_OPTION Phase_Stats_Control(int32_t nframes);
static void Phase_Stats_Draw(void);

static void Phase_Stats_CreateTexts(int32_t level_num)
{
    char buf[100];
    char time_str[100];

    const GAME_STATS *stats = &g_GameInfo.current[level_num].stats;

    Overlay_HideGameInfo();

    int y = -50;
    const int row_height = 30;

    TEXTSTRING **cur_txt = &m_Texts[0];

    // heading
    sprintf(buf, "%s", g_GameFlow.levels[level_num].level_title);
    *cur_txt = Text_Create(0, y, buf);
    Text_CentreH(*cur_txt, 1);
    Text_CentreV(*cur_txt, 1);
    cur_txt++;
    y += row_height;

    // kills
    sprintf(
        buf,
        g_GameFlow.strings
            [g_Config.enable_detailed_stats ? GS_STATS_KILLS_DETAIL_FMT
                                            : GS_STATS_KILLS_BASIC_FMT],
        stats->kill_count, stats->max_kill_count);
    *cur_txt = Text_Create(0, y, buf);
    Text_CentreH(*cur_txt, 1);
    Text_CentreV(*cur_txt, 1);
    cur_txt++;
    y += row_height;

    // pickups
    sprintf(
        buf,
        g_GameFlow.strings
            [g_Config.enable_detailed_stats ? GS_STATS_PICKUPS_DETAIL_FMT
                                            : GS_STATS_PICKUPS_BASIC_FMT],
        stats->pickup_count, stats->max_pickup_count);
    *cur_txt = Text_Create(0, y, buf);
    Text_CentreH(*cur_txt, 1);
    Text_CentreV(*cur_txt, 1);
    cur_txt++;
    y += row_height;

    // secrets
    int secret_count = 0;
    int16_t secret_flags = stats->secret_flags;
    for (int i = 0; i < MAX_SECRETS; i++) {
        if (secret_flags & 1) {
            secret_count++;
        }
        secret_flags >>= 1;
    }
    sprintf(
        buf, g_GameFlow.strings[GS_STATS_SECRETS_FMT], secret_count,
        stats->max_secret_count);
    *cur_txt = Text_Create(0, y, buf);
    Text_CentreH(*cur_txt, 1);
    Text_CentreV(*cur_txt, 1);
    cur_txt++;
    y += row_height;

    // deaths
    if (g_Config.enable_deaths_counter && g_GameInfo.death_counter_supported) {
        sprintf(
            buf, g_GameFlow.strings[GS_STATS_DEATHS_FMT], stats->death_count);
        *cur_txt = Text_Create(0, y, buf);
        Text_CentreH(*cur_txt, 1);
        Text_CentreV(*cur_txt, 1);
        cur_txt++;
        y += row_height;
    }

    // time taken
    int seconds = stats->timer / 30;
    int hours = seconds / 3600;
    int minutes = (seconds / 60) % 60;
    seconds %= 60;
    if (hours) {
        sprintf(
            time_str, "%d:%d%d:%d%d", hours, minutes / 10, minutes % 10,
            seconds / 10, seconds % 10);
    } else {
        sprintf(time_str, "%d:%d%d", minutes, seconds / 10, seconds % 10);
    }
    sprintf(buf, g_GameFlow.strings[GS_STATS_TIME_TAKEN_FMT], time_str);
    *cur_txt = Text_Create(0, y, buf);
    Text_CentreH(*cur_txt, 1);
    Text_CentreV(*cur_txt, 1);
    cur_txt++;
    y += row_height;
}

static void Phase_Stats_Start(void *arg)
{
    const PHASE_STATS_DATA *data = (const PHASE_STATS_DATA *)arg;
    if (g_CurrentLevel == g_GameFlow.gym_level_num) {
        Output_FadeToBlack(false);
        m_State = STATS_STATE_FADE_OUT;
        return;
    }

    m_State = STATS_STATE_FADE_IN;
    m_OldGameStatus = Game_GetStatus();
    Output_FadeToSemiBlack(true);

    Phase_Stats_CreateTexts(
        data && data->level_num != -1 ? data->level_num : g_CurrentLevel);
}

static void Phase_Stats_End(void)
{
    Output_FadeReset();

    for (int i = 0; i < MAX_TEXTSTRINGS; i++) {
        TEXTSTRING **cur_txt = &m_Texts[i];
        if (*cur_txt) {
            Text_Remove(*cur_txt);
        }
    }
}

static GAMEFLOW_OPTION Phase_Stats_Control(int32_t nframes)
{
    Input_Update();
    Shell_ProcessInput();

    switch (m_State) {
    case STATS_STATE_FADE_IN:
        if (!Output_FadeIsAnimating() || g_InputDB.menu_confirm
            || g_InputDB.menu_back) {
            m_State = STATS_STATE_DISPLAY;
        }
        break;

    case STATS_STATE_DISPLAY:
        if (g_InputDB.menu_confirm || g_InputDB.menu_back) {
            m_State = STATS_STATE_FADE_OUT;
        }
        break;

    case STATS_STATE_FADE_OUT:
        Output_FadeToBlack(true);
        if (!Output_FadeIsAnimating() || g_InputDB.menu_confirm
            || g_InputDB.menu_back) {
            Game_SetStatus(m_OldGameStatus);
            return GF_NOP_BREAK;
        }
        break;
    }

    return GF_NOP;
}

static void Phase_Stats_Draw(void)
{
    Game_DrawScene(false);
    Text_Draw();
}

PHASER g_StatsPhaser = {
    .start = Phase_Stats_Start,
    .end = Phase_Stats_End,
    .control = Phase_Stats_Control,
    .draw = Phase_Stats_Draw,
};
