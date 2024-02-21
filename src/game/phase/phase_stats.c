#include "game/phase/phase_stats.h"

#include "config.h"
#include "game/game.h"
#include "game/gameflow.h"
#include "game/input.h"
#include "game/music.h"
#include "game/output.h"
#include "game/overlay.h"
#include "game/phase/phase.h"
#include "game/shell.h"
#include "game/stats.h"
#include "game/text.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define MAX_TEXTSTRINGS 10

typedef enum STATS_STATE {
    STATS_STATE_FADE_IN,
    STATS_STATE_DISPLAY,
    STATS_STATE_FADE_OUT,
} STATS_STATE;

static bool m_Total = false;
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

static void Phase_Stats_CreateTextsTotal(GAMEFLOW_LEVEL_TYPE level_type)
{
    TOTAL_STATS stats;
    Stats_ComputeTotal(level_type, &stats);

    char buf[100];
    char time_str[100];
    TEXTSTRING **cur_txt = &m_Texts[0];

    int top_y = 55;
    int y = 55;
    const int row_width = 220;
    const int row_height = 20;
    int16_t border = 4;

    // reserve space for heading
    y += row_height + border * 2;

    // kills
    sprintf(
        buf,
        g_GameFlow.strings
            [g_Config.enable_detailed_stats ? GS_STATS_KILLS_DETAIL_FMT
                                            : GS_STATS_KILLS_BASIC_FMT],
        stats.player_kill_count, stats.total_kill_count);
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
        stats.player_pickup_count, stats.total_pickup_count);
    *cur_txt = Text_Create(0, y, buf);
    Text_CentreH(*cur_txt, 1);
    Text_CentreV(*cur_txt, 1);
    cur_txt++;
    y += row_height;

    // secrets
    sprintf(
        buf, g_GameFlow.strings[GS_STATS_SECRETS_FMT],
        stats.player_secret_count, stats.total_secret_count);
    *cur_txt = Text_Create(0, y, buf);
    Text_CentreH(*cur_txt, 1);
    Text_CentreV(*cur_txt, 1);
    cur_txt++;
    y += row_height;

    // deaths
    if (g_Config.enable_deaths_counter && g_GameInfo.death_counter_supported) {
        sprintf(
            buf, g_GameFlow.strings[GS_STATS_DEATHS_FMT], stats.death_count);
        *cur_txt = Text_Create(0, y, buf);
        Text_CentreH(*cur_txt, 1);
        Text_CentreV(*cur_txt, 1);
        cur_txt++;
        y += row_height;
    }

    // time taken
    int seconds = stats.timer / 30;
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

    // border
    int16_t height = y + border * 2 - top_y;
    *cur_txt = Text_Create(0, top_y, " ");
    Text_CentreH(*cur_txt, 1);
    Text_CentreV(*cur_txt, 1);
    Text_AddBackground(*cur_txt, row_width, height, 0, 0, TS_BACKGROUND);
    Text_AddOutline(*cur_txt, true, TS_BACKGROUND);
    cur_txt++;

    // heading
    sprintf(
        buf, "%s",
        g_GameFlow.strings
            [level_type == GFL_BONUS ? GS_STATS_BONUS_STATISTICS
                                     : GS_STATS_FINAL_STATISTICS]);
    *cur_txt = Text_Create(0, top_y + 2, buf);
    Text_CentreH(*cur_txt, 1);
    Text_CentreV(*cur_txt, 1);
    Text_AddBackground(*cur_txt, row_width - 4, 0, 0, 0, TS_HEADING);
    Text_AddOutline(*cur_txt, true, TS_HEADING);
    cur_txt++;
}

static void Phase_Stats_Start(void *arg)
{
    const PHASE_STATS_DATA *data = (const PHASE_STATS_DATA *)arg;
    if (data && data->total) {
        assert(data->level_type);
        Output_LoadBackdropImage(data->background_path);
    }

    if (g_CurrentLevel == g_GameFlow.gym_level_num) {
        Output_FadeToBlack(false);
        m_State = STATS_STATE_FADE_OUT;
        return;
    }

    m_State = STATS_STATE_FADE_IN;
    m_Total = data && data->total;

    if (data && data->total) {
        Phase_Stats_CreateTextsTotal(data->level_type);
        Output_FadeReset();
        Output_FadeResetToBlack();
        Output_FadeToTransparent(true);
    } else {
        Phase_Stats_CreateTexts(
            data && data->level_num != -1 ? data->level_num : g_CurrentLevel);
        Output_FadeToSemiBlack(true);
    }
}

static void Phase_Stats_End(void)
{
    Music_Stop();

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
        if (!Output_FadeIsAnimating()) {
            m_State = STATS_STATE_DISPLAY;
        } else if (g_InputDB.menu_confirm || g_InputDB.menu_back) {
            m_State = STATS_STATE_FADE_OUT;
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
            return GF_NOP_BREAK;
        }
        break;
    }

    return GF_NOP;
}

static void Phase_Stats_Draw(void)
{
    if (m_Total) {
        Output_DrawBackdropImage();
    } else {
        Game_DrawScene(false);
    }
    Text_Draw();
}

PHASER g_StatsPhaser = {
    .start = Phase_Stats_Start,
    .end = Phase_Stats_End,
    .control = Phase_Stats_Control,
    .draw = Phase_Stats_Draw,
    .wait = NULL,
};
