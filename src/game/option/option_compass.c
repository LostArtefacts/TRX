#include "game/option/option_compass.h"

#include "config.h"
#include "game/difficulty.h"
#include "game/gameflow.h"
#include "game/input.h"
#include "game/text.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef enum COMPASS_TEXT {
    TEXT_TITLE = 0,
    TEXT_TITLE_BORDER = 1,
    TEXT_TIME = 2,
    TEXT_SECRETS = 3,
    TEXT_PICKUPS = 4,
    TEXT_DEATHS = 5,
    TEXT_KILLS = 6,
    TEXT_DIFFICULTY = 7,
    TEXT_NUMBER_OF = 8,
} COMPASS_TEXT;

static TEXTSTRING *m_Text[TEXT_NUMBER_OF] = { 0 };

static void Option_CompassInitText(void);

static void Option_CompassInitText(void)
{
    char buf[100];
    const int top_y = -60;
    const int border = 4;
    const int row_height = 25;
    const int row_width = 225;
    const GAME_STATS *stats = &g_GameInfo.current[g_CurrentLevel].stats;

    int y = top_y;
    m_Text[TEXT_TITLE_BORDER] = Text_Create(0, y - border, " ");
    Text_CentreH(m_Text[TEXT_TITLE_BORDER], true);
    Text_CentreV(m_Text[TEXT_TITLE_BORDER], true);

    sprintf(buf, "%s", g_GameFlow.levels[g_CurrentLevel].level_title);
    m_Text[TEXT_TITLE] = Text_Create(0, y - border / 2, buf);
    Text_CentreH(m_Text[TEXT_TITLE], true);
    Text_CentreV(m_Text[TEXT_TITLE], true);
    y += row_height;

    // kills
    sprintf(
        buf,
        g_GameFlow.strings
            [g_Config.enable_detailed_stats ? GS_STATS_KILLS_DETAIL_FMT
                                            : GS_STATS_KILLS_BASIC_FMT],
        stats->kill_count, stats->max_kill_count);
    m_Text[TEXT_KILLS] = Text_Create(0, y, buf);
    y += row_height;

    // pickups
    sprintf(
        buf,
        g_GameFlow.strings
            [g_Config.enable_detailed_stats ? GS_STATS_PICKUPS_DETAIL_FMT
                                            : GS_STATS_PICKUPS_BASIC_FMT],
        stats->pickup_count, stats->max_pickup_count);
    m_Text[TEXT_PICKUPS] = Text_Create(0, y, buf);
    y += row_height;

    // secrets
    int secret_count = 0;
    int secret_flags = stats->secret_flags;
    for (int i = 0; i < MAX_SECRETS; i++) {
        if (secret_flags & 1) {
            secret_count++;
        }
        secret_flags >>= 1;
    }
    sprintf(
        buf, g_GameFlow.strings[GS_STATS_SECRETS_FMT], secret_count,
        g_GameInfo.current[g_CurrentLevel].stats.max_secret_count);
    m_Text[TEXT_SECRETS] = Text_Create(0, y, buf);
    y += row_height;

    // deaths
    if (g_Config.enable_deaths_counter && g_GameInfo.death_counter_supported) {
        sprintf(
            buf, g_GameFlow.strings[GS_STATS_DEATHS_FMT], stats->death_count);
        m_Text[TEXT_DEATHS] = Text_Create(0, y, buf);
        y += row_height;
    }

    // difficulty
    if (g_Config.enable_difficulty) {
        Difficulty_GetTextStat(buf, g_Config.damages_to_lara_multiplier);
        m_Text[TEXT_DIFFICULTY] = Text_Create(0, y, buf);
        y += row_height;
    }

    // time taken
    m_Text[TEXT_TIME] = Text_Create(0, y, " ");
    y += row_height;

    Text_AddBackground(
        m_Text[TEXT_TITLE_BORDER], row_width, y - top_y, 0, 0, TS_BACKGROUND);
    Text_AddOutline(m_Text[TEXT_TITLE_BORDER], true, TS_BACKGROUND);
    Text_AddBackground(m_Text[TEXT_TITLE], row_width - 4, 0, 0, 0, TS_HEADING);
    Text_AddOutline(m_Text[TEXT_TITLE], true, TS_HEADING);

    for (int i = 0; i < TEXT_NUMBER_OF; i++) {
        Text_CentreH(m_Text[i], 1);
        Text_CentreV(m_Text[i], 1);
    }
}

void Option_Compass(INVENTORY_ITEM *inv_item)
{
    if (g_Config.enable_compass_stats) {
        char buf[100];
        char time_buf[100];

        if (!m_Text[0]) {
            Option_CompassInitText();
        }

        int32_t seconds = g_GameInfo.current[g_CurrentLevel].stats.timer / 30;
        int32_t hours = seconds / 3600;
        int32_t minutes = (seconds / 60) % 60;
        seconds %= 60;
        if (hours) {
            sprintf(
                time_buf, "%d:%d%d:%d%d", hours, minutes / 10, minutes % 10,
                seconds / 10, seconds % 10);
        } else {
            sprintf(time_buf, "%d:%d%d", minutes, seconds / 10, seconds % 10);
        }
        sprintf(buf, g_GameFlow.strings[GS_STATS_TIME_TAKEN_FMT], time_buf);
        Text_ChangeText(m_Text[TEXT_TIME], buf);
    }

    if (g_InputDB.deselect || g_InputDB.select) {
        for (int i = 0; i < TEXT_NUMBER_OF; i++) {
            Text_Remove(m_Text[i]);
            m_Text[i] = NULL;
        }
        inv_item->goal_frame = inv_item->frames_total - 1;
        inv_item->anim_direction = 1;
    }
}
