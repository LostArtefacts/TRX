#include "game/difficulty.h"

#include "config.h"
#include "game/gameflow.h"
#include "game/input.h"
#include "game/text.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

int16_t Difficulty_GetCurrentIndex(float damages_to_lara_multiplier)
{
    for (int i = 0; i < MAX_DIFFICULTY_PRESETS; i++) {
        if (damages_to_lara_multiplier == g_Difficulty_Presets[i]) {
            return i;
        }
    }
    return -1;
}

void Difficulty_GetTextStat(char *str, float damages_to_lara_multiplier)
{
    if (g_Config.enable_cheats) {
        sprintf(
            str, g_GameFlow.strings[GS_STATS_DIFFICULTY_FMT],
            g_GameFlow.strings[GS_DIFFICULTY_C]);
        return;
    }

    int16_t current_index =
        Difficulty_GetCurrentIndex(damages_to_lara_multiplier);
    switch (current_index) {
        char str_user_difficulty[50];
    case 0:
        sprintf(
            str, g_GameFlow.strings[GS_STATS_DIFFICULTY_FMT],
            g_GameFlow.strings[GS_DIFFICULTY_1]);
        break;
    case 1:
        sprintf(
            str, g_GameFlow.strings[GS_STATS_DIFFICULTY_FMT],
            g_GameFlow.strings[GS_DIFFICULTY_2]);
        break;
    case 2:
        sprintf(
            str, g_GameFlow.strings[GS_STATS_DIFFICULTY_FMT],
            g_GameFlow.strings[GS_DIFFICULTY_3]);
        break;
    case 3:
        sprintf(
            str, g_GameFlow.strings[GS_STATS_DIFFICULTY_FMT],
            g_GameFlow.strings[GS_DIFFICULTY_4]);
        break;
    case 4:
        sprintf(
            str, g_GameFlow.strings[GS_STATS_DIFFICULTY_FMT],
            g_GameFlow.strings[GS_DIFFICULTY_5]);
        break;
    case 5:
        sprintf(
            str, g_GameFlow.strings[GS_STATS_DIFFICULTY_FMT],
            g_GameFlow.strings[GS_DIFFICULTY_6]);
        break;
    case 6:
        sprintf(
            str, g_GameFlow.strings[GS_STATS_DIFFICULTY_FMT],
            g_GameFlow.strings[GS_DIFFICULTY_7]);
        break;
    default:
        sprintf(
            str_user_difficulty, "%s %.1fx",
            g_GameFlow.strings[GS_DIFFICULTY_0], damages_to_lara_multiplier);
        sprintf(
            str, g_GameFlow.strings[GS_STATS_DIFFICULTY_FMT],
            str_user_difficulty);
        break;
    }
}

void Difficulty_GetTextStat_NoHeader(
    char *str, float damages_to_lara_multiplier)
{
    int16_t current_index =
        Difficulty_GetCurrentIndex(damages_to_lara_multiplier);
    switch (current_index) {
        char str_user_difficulty[50];
    case 0:
        sprintf(str, "%s", g_GameFlow.strings[GS_DIFFICULTY_1]);
        break;
    case 1:
        sprintf(str, "%s", g_GameFlow.strings[GS_DIFFICULTY_2]);
        break;
    case 2:
        sprintf(str, "%s", g_GameFlow.strings[GS_DIFFICULTY_3]);
        break;
    case 3:
        sprintf(str, "%s", g_GameFlow.strings[GS_DIFFICULTY_4]);
        break;
    case 4:
        sprintf(str, "%s", g_GameFlow.strings[GS_DIFFICULTY_5]);
        break;
    case 5:
        sprintf(str, "%s", g_GameFlow.strings[GS_DIFFICULTY_6]);
        break;
    case 6:
        sprintf(str, "%s", g_GameFlow.strings[GS_DIFFICULTY_7]);
        break;
    default:
        sprintf(
            str_user_difficulty, "%s %.1fx",
            g_GameFlow.strings[GS_DIFFICULTY_0], damages_to_lara_multiplier);
        sprintf(
            str, g_GameFlow.strings[GS_STATS_DIFFICULTY_FMT],
            str_user_difficulty);
        break;
    }
}

void Difficulty_Select(int16_t difficulty_select_mode)
{

    if (!g_Config.enable_difficulty) {
        return;
    }
    static TEXTSTRING *m_Text;
    static TEXTSTRING *m_Text_moreup;
    static TEXTSTRING *m_Text_moredown;
    static float next_damages_to_lara_multiplier = -1;
    char buf[100];

    switch (difficulty_select_mode) {
    case DIFFICULTY_SELECT_SHUTDOWN:
        Text_Remove(m_Text);
        Text_Remove(m_Text_moreup);
        Text_Remove(m_Text_moredown);
        m_Text = NULL;
        m_Text_moreup = NULL;
        m_Text_moredown = NULL;
        break;
    case DIFFICULTY_SELECT_INIT:
        next_damages_to_lara_multiplier = g_Config.damages_to_lara_multiplier;
        Difficulty_GetTextStat_NoHeader(buf, next_damages_to_lara_multiplier);
        m_Text = Text_Create(0, -54, buf);
        m_Text_moreup = Text_Create(0, -69, "[");
        m_Text_moredown = Text_Create(0, -44, "]");
        Text_Hide(m_Text, true);
        Text_Hide(m_Text_moreup, true);
        Text_Hide(m_Text_moredown, true);
        Text_AlignBottom(m_Text, 1);
        Text_AlignBottom(m_Text_moreup, 1);
        Text_AlignBottom(m_Text_moredown, 1);
        Text_CentreH(m_Text, 1);
        Text_CentreH(m_Text_moreup, 1);
        Text_CentreH(m_Text_moredown, 1);
        Text_SetScale(m_Text, PHD_ONE * 0.95, PHD_ONE * 0.95);
        Text_SetScale(m_Text_moreup, PHD_ONE * 0.55, PHD_ONE * 0.55);
        Text_SetScale(m_Text_moredown, PHD_ONE * 0.55, PHD_ONE * 0.55);
        break;
    case DIFFICULTY_SELECT_HIDEALL:
        Text_Hide(m_Text, true);
        Text_Hide(m_Text_moreup, true);
        Text_Hide(m_Text_moredown, true);
        return;
    case DIFFICULTY_SELECT_WAITING_INPUT:
        Text_Hide(m_Text, false);
        Text_SetScale(m_Text, PHD_ONE * 0.95, PHD_ONE * 0.95);
        break;
    case DIFFICULTY_SELECT_IN_BACKGROUND:
        return; // ugly but temporary fix agaisnt overlapping text with
                // savegames list. WIP
        Text_Hide(m_Text, false);
        Text_SetScale(m_Text, PHD_ONE * 0.88, PHD_ONE * 0.88);
        return;
    case DIFFICULTY_SELECT_CONFIRM: // prevents changing diff. during a game
        if (next_damages_to_lara_multiplier > 0) { // important
            g_Config.damages_to_lara_multiplier =
                next_damages_to_lara_multiplier;
        }
        return;
    default:
        return;
    }

    int16_t current_index =
        Difficulty_GetCurrentIndex(next_damages_to_lara_multiplier);

    if (current_index > 0) {
        Text_Hide(m_Text_moreup, false);
    } else {
        Text_Hide(m_Text_moreup, true);
    }

    if (current_index < MAX_DIFFICULTY_PRESETS - 1) {
        Text_Hide(m_Text_moredown, false);
    } else {
        Text_Hide(m_Text_moredown, true);
    }

    if (g_InputDB.forward) {
        g_InputDB = (INPUT_STATE) { 0 };
        if (current_index > 0) {
            current_index--;
            // g_Config.damages_to_lara_multiplier =
            // g_Difficulty_Presets[current_index];
            next_damages_to_lara_multiplier =
                g_Difficulty_Presets[current_index];
            Difficulty_GetTextStat_NoHeader(
                buf, next_damages_to_lara_multiplier);
            Text_ChangeText(m_Text, buf);
        }
    } else if (g_InputDB.back) {
        g_InputDB = (INPUT_STATE) { 0 };
        if (current_index < MAX_DIFFICULTY_PRESETS - 1) {
            current_index++;
            // g_Config.damages_to_lara_multiplier =
            // g_Difficulty_Presets[current_index];
            next_damages_to_lara_multiplier =
                g_Difficulty_Presets[current_index];
            Difficulty_GetTextStat_NoHeader(
                buf, next_damages_to_lara_multiplier);
            Text_ChangeText(m_Text, buf);
        }
    }
}
