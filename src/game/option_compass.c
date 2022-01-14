#include "game/option.h"

#include "config.h"
#include "game/gameflow.h"
#include "game/input.h"
#include "game/text.h"
#include "global/vars.h"

#include <stdio.h>

#define TOP_Y -100
#define ROW_HEIGHT 25
#define ROW_WIDTH 225

typedef enum COMPASS_TEXT {
    TEXT_TITLE = 0,
    TEXT_TITLE_BORDER = 1,
    TEXT_TIME = 2,
    TEXT_SECRETS = 3,
    TEXT_PICKUPS = 4,
    TEXT_KILLS = 5,
    TEXT_NUMBER_OF = 6,
} COMPASS_TEXT;

static TEXTSTRING *m_Text[TEXT_NUMBER_OF] = { 0 };

static void Option_CompassInitText();

static void Option_CompassInitText()
{
    char buf[100];
    int32_t y = TOP_Y;

    m_Text[TEXT_TITLE_BORDER] = Text_Create(0, y - 2, " ");

    sprintf(buf, "%s", g_GameFlow.levels[g_CurrentLevel].level_title);
    m_Text[TEXT_TITLE] = Text_Create(0, y, buf);
    y += ROW_HEIGHT;

    m_Text[TEXT_TIME] = Text_Create(0, y, " ");
    y += ROW_HEIGHT;

    int32_t secrets_taken = 0;
    int32_t secrets_total = MAX_SECRETS;
    int32_t secrets_flags = g_SaveGame.secrets;
    do {
        if (secrets_flags & 1) {
            secrets_taken++;
        }
        secrets_flags >>= 1;
        secrets_total--;
    } while (secrets_total);
    sprintf(
        buf, g_GameFlow.strings[GS_STATS_SECRETS_FMT], secrets_taken,
        g_GameFlow.levels[g_CurrentLevel].secrets);
    m_Text[TEXT_SECRETS] = Text_Create(0, y, buf);
    y += ROW_HEIGHT;

    sprintf(buf, g_GameFlow.strings[GS_STATS_PICKUPS_FMT], g_SaveGame.pickups);
    m_Text[TEXT_PICKUPS] = Text_Create(0, y, buf);
    y += ROW_HEIGHT;

    sprintf(buf, g_GameFlow.strings[GS_STATS_KILLS_FMT], g_SaveGame.kills);
    m_Text[TEXT_KILLS] = Text_Create(0, y, buf);
    y += ROW_HEIGHT;

    Text_AddBackground(m_Text[TEXT_TITLE_BORDER], ROW_WIDTH, y - TOP_Y, 0, 0);
    Text_AddOutline(m_Text[TEXT_TITLE_BORDER], 1);
    Text_AddBackground(m_Text[TEXT_TITLE], ROW_WIDTH - 4, 0, 0, 0);
    Text_AddOutline(m_Text[TEXT_TITLE], 1);

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

        int32_t seconds = g_SaveGame.timer / 30;
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
