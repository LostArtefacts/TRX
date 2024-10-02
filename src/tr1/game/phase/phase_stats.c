#include "game/phase/phase_stats.h"

#include "config.h"
#include "game/game.h"
#include "game/game_string.h"
#include "game/gameflow.h"
#include "game/input.h"
#include "game/interpolation.h"
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

typedef enum {
    STATE_FADE_IN,
    STATE_DISPLAY,
    STATE_FADE_OUT,
} STATE;

static bool m_Total = false;
static STATE m_State = STATE_DISPLAY;
static TEXTSTRING *m_Texts[MAX_TEXTSTRINGS] = { 0 };

static void M_CreateTexts(int32_t level_num);
static void M_CreateTextsTotal(GAMEFLOW_LEVEL_TYPE level_type);
static void M_Start(void *arg);
static void M_End(void);
static PHASE_CONTROL M_Control(int32_t nframes);
static void M_Draw(void);

static void M_CreateTexts(int32_t level_num)
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
        g_Config.enable_detailed_stats ? GS(STATS_KILLS_DETAIL_FMT)
                                       : GS(STATS_KILLS_BASIC_FMT),
        stats->kill_count, stats->max_kill_count);
    *cur_txt = Text_Create(0, y, buf);
    Text_CentreH(*cur_txt, 1);
    Text_CentreV(*cur_txt, 1);
    cur_txt++;
    y += row_height;

    // pickups
    sprintf(
        buf,
        g_Config.enable_detailed_stats ? GS(STATS_PICKUPS_DETAIL_FMT)
                                       : GS(STATS_PICKUPS_BASIC_FMT),
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
    sprintf(buf, GS(STATS_SECRETS_FMT), secret_count, stats->max_secret_count);
    *cur_txt = Text_Create(0, y, buf);
    Text_CentreH(*cur_txt, 1);
    Text_CentreV(*cur_txt, 1);
    cur_txt++;
    y += row_height;

    // deaths
    if (g_Config.enable_deaths_counter && g_GameInfo.death_counter_supported) {
        sprintf(buf, GS(STATS_DEATHS_FMT), stats->death_count);
        *cur_txt = Text_Create(0, y, buf);
        Text_CentreH(*cur_txt, 1);
        Text_CentreV(*cur_txt, 1);
        cur_txt++;
        y += row_height;
    }

    // time taken
    int seconds = stats->timer / LOGIC_FPS;
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
    sprintf(buf, GS(STATS_TIME_TAKEN_FMT), time_str);
    *cur_txt = Text_Create(0, y, buf);
    Text_CentreH(*cur_txt, 1);
    Text_CentreV(*cur_txt, 1);
    cur_txt++;
    y += row_height;
}

static void M_CreateTextsTotal(GAMEFLOW_LEVEL_TYPE level_type)
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
        g_Config.enable_detailed_stats ? GS(STATS_KILLS_DETAIL_FMT)
                                       : GS(STATS_KILLS_BASIC_FMT),
        stats.player_kill_count, stats.total_kill_count);
    *cur_txt = Text_Create(0, y, buf);
    Text_CentreH(*cur_txt, 1);
    Text_CentreV(*cur_txt, 1);
    cur_txt++;
    y += row_height;

    // pickups
    sprintf(
        buf,
        g_Config.enable_detailed_stats ? GS(STATS_PICKUPS_DETAIL_FMT)
                                       : GS(STATS_PICKUPS_BASIC_FMT),
        stats.player_pickup_count, stats.total_pickup_count);
    *cur_txt = Text_Create(0, y, buf);
    Text_CentreH(*cur_txt, 1);
    Text_CentreV(*cur_txt, 1);
    cur_txt++;
    y += row_height;

    // secrets
    sprintf(
        buf, GS(STATS_SECRETS_FMT), stats.player_secret_count,
        stats.total_secret_count);
    *cur_txt = Text_Create(0, y, buf);
    Text_CentreH(*cur_txt, 1);
    Text_CentreV(*cur_txt, 1);
    cur_txt++;
    y += row_height;

    // deaths
    if (g_Config.enable_deaths_counter && g_GameInfo.death_counter_supported) {
        sprintf(buf, GS(STATS_DEATHS_FMT), stats.death_count);
        *cur_txt = Text_Create(0, y, buf);
        Text_CentreH(*cur_txt, 1);
        Text_CentreV(*cur_txt, 1);
        cur_txt++;
        y += row_height;
    }

    // time taken
    int seconds = stats.timer / LOGIC_FPS;
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
    sprintf(buf, GS(STATS_TIME_TAKEN_FMT), time_str);
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
        level_type == GFL_BONUS ? GS(STATS_BONUS_STATISTICS)
                                : GS(STATS_FINAL_STATISTICS));
    *cur_txt = Text_Create(0, top_y + 2, buf);
    Text_CentreH(*cur_txt, 1);
    Text_CentreV(*cur_txt, 1);
    Text_AddBackground(*cur_txt, row_width - 4, 0, 0, 0, TS_HEADING);
    Text_AddOutline(*cur_txt, true, TS_HEADING);
    cur_txt++;
}

static void M_Start(void *arg)
{
    const PHASE_STATS_DATA *data = (const PHASE_STATS_DATA *)arg;
    if (data && data->total) {
        assert(data->level_type);
        Output_LoadBackdropImage(data->background_path);
    } else {
        Output_LoadBackdropImage(NULL);
    }

    if (g_CurrentLevel == g_GameFlow.gym_level_num) {
        Output_FadeToBlack(false);
        m_State = STATE_FADE_OUT;
        return;
    }

    m_State = STATE_FADE_IN;
    m_Total = data && data->total;

    if (data && data->total) {
        M_CreateTextsTotal(data->level_type);
        Output_FadeReset();
        Output_FadeResetToBlack();
        Output_FadeToTransparent(true);
    } else {
        M_CreateTexts(
            data && data->level_num != -1 ? data->level_num : g_CurrentLevel);
        Output_FadeToSemiBlack(true);
    }
}

static void M_End(void)
{
    Music_Stop();

    for (int i = 0; i < MAX_TEXTSTRINGS; i++) {
        TEXTSTRING **cur_txt = &m_Texts[i];
        if (*cur_txt) {
            Text_Remove(*cur_txt);
        }
    }
}

static PHASE_CONTROL M_Control(int32_t nframes)
{
    Input_Update();
    Shell_ProcessInput();

    switch (m_State) {
    case STATE_FADE_IN:
        if (!Output_FadeIsAnimating()) {
            m_State = STATE_DISPLAY;
        } else if (g_InputDB.menu_confirm || g_InputDB.menu_back) {
            m_State = STATE_FADE_OUT;
        }
        break;

    case STATE_DISPLAY:
        if (g_InputDB.menu_confirm || g_InputDB.menu_back) {
            m_State = STATE_FADE_OUT;
        }
        break;

    case STATE_FADE_OUT:
        Output_FadeToBlack(true);
        if (g_InputDB.menu_confirm || g_InputDB.menu_back
            || !Output_FadeIsAnimating()) {
            Output_FadeResetToBlack();
            return (PHASE_CONTROL) {
                .end = true,
                .command = { .action = GF_CONTINUE_SEQUENCE },
            };
        }
        break;
    }

    return (PHASE_CONTROL) { .end = false };
}

static void M_Draw(void)
{
    if (!m_Total) {
        Interpolation_Disable();
        Game_DrawScene(false);
        Interpolation_Enable();
    }
    Output_AnimateFades();
    Text_Draw();
}

PHASER g_StatsPhaser = {
    .start = M_Start,
    .end = M_End,
    .control = M_Control,
    .draw = M_Draw,
    .wait = NULL,
};
