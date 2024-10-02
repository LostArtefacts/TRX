#include "decomp/stats.h"

#include "game/input.h"
#include "game/music.h"
#include "game/overlay.h"
#include "game/requester.h"
#include "game/text.h"
#include "global/funcs.h"
#include "global/types.h"
#include "global/vars.h"

#include <libtrx/log.h>

#include <stdio.h>

// TODO: consolidate with STATISTICS_INFO
typedef struct {
    uint32_t timer;
    uint32_t ammo_used;
    uint32_t ammo_hits;
    uint32_t distance;
    uint32_t kills;
    uint8_t found_secrets; // this is no longer a bitmask
    uint8_t total_secrets; // this is not present in STATISTICS_INFO
    uint8_t medipacks;
} STATS;

static STATS M_GetEndGameStats(void)
{
    STATS result = { 0 };

    const int32_t total_levels = g_GameFlow.num_levels - g_GameFlow.num_demos;
    for (int32_t i = LV_FIRST; i < total_levels; i++) {
        result.timer += g_SaveGame.start[i].statistics.timer;
        result.ammo_used += g_SaveGame.start[i].statistics.shots;
        result.ammo_hits += g_SaveGame.start[i].statistics.hits;
        result.kills += g_SaveGame.start[i].statistics.kills;
        result.distance += g_SaveGame.start[i].statistics.distance;
        result.medipacks += g_SaveGame.start[i].statistics.medipacks;

        // TODO: #170, consult GFE_NUM_SECRETS rather than hardcoding this
        if (i < total_levels - 2) {
            for (int32_t j = 0; j < 3; j++) {
                if (g_SaveGame.start[i].statistics.secrets & (1 << j)) {
                    result.found_secrets++;
                }
                result.total_secrets++;
            }
        }
    }

    return result;
}

void __cdecl ShowGymStatsText(const char *const time_str, const int32_t type)
{
    char text1[32];
    char text2[32];

    if (g_StatsRequester.ready) {
        if (!Requester_Display(&g_StatsRequester, 1, 1)) {
            g_InputDB = 0;
            g_Input = 0;
        }
        return;
    }

    g_StatsRequester.no_selector = 1;
    Requester_SetSize(&g_StatsRequester, 7, -32);
    g_StatsRequester.line_height = 18;
    g_StatsRequester.items_count = 0;
    g_StatsRequester.selected = 0;
    g_StatsRequester.line_offset = 0;
    g_StatsRequester.line_old_offset = 0;
    g_StatsRequester.pix_width = 304;
    g_StatsRequester.x_pos = 0;
    g_StatsRequester.z_pos = 0;
    g_StatsRequester.pitem_strings1 = g_ValidLevelStrings1;
    g_StatsRequester.pitem_strings2 = g_ValidLevelStrings2;
    g_StatsRequester.item_string_len = MAX_LEVEL_NAME_SIZE;

    Requester_Init(&g_StatsRequester);
    Requester_SetHeading(
        &g_StatsRequester, g_GF_GameStrings[GF_S_GAME_MISC_BEST_TIMES],
        REQ_CENTER, NULL, 0);

    int32_t i;
    for (i = 0; i < 10; i++) {
        if (!g_Assault.best_time[i]) {
            break;
        }

        sprintf(
            text1, "%2d: %s %d", i + 1, g_GF_GameStrings[GF_S_GAME_MISC_FINISH],
            g_Assault.best_finish[i]);
        const int32_t sec = g_Assault.best_time[i] / FRAMES_PER_SECOND;
        sprintf(
            text2, "%02d:%02d.%-2d", sec / 60, sec % 60,
            g_Assault.best_time[i] % FRAMES_PER_SECOND
                / (FRAMES_PER_SECOND / 10));
        Requester_AddItem(
            &g_StatsRequester, text1, REQ_ALIGN_LEFT, text2, REQ_ALIGN_RIGHT);
    }

    if (i == 0) {
        Requester_AddItem(
            &g_StatsRequester, g_GF_GameStrings[GF_S_GAME_MISC_NO_TIMES_SET],
            REQ_CENTER, NULL, REQ_CENTER);
    }

    g_StatsRequester.ready = 1;
}

void __cdecl ShowStatsText(const char *const time_str, const int32_t type)
{
    char buffer[32];

    if (g_StatsRequester.ready) {
        Requester_ChangeItem(
            &g_StatsRequester, 0, g_GF_GameStrings[GF_S_GAME_MISC_TIME_TAKEN],
            REQ_ALIGN_LEFT, time_str, REQ_ALIGN_RIGHT);
        if (!Requester_Display(&g_StatsRequester, type, 1)) {
            g_InputDB = 0;
            g_Input = 0;
        }
        return;
    }

    g_StatsRequester.no_selector = 1;
    Requester_SetSize(&g_StatsRequester, 7, -32);
    g_StatsRequester.line_height = 18;
    g_StatsRequester.items_count = 0;
    g_StatsRequester.selected = 0;
    g_StatsRequester.line_offset = 0;
    g_StatsRequester.line_old_offset = 0;
    g_StatsRequester.pix_width = 304;
    g_StatsRequester.x_pos = 0;
    g_StatsRequester.z_pos = 0;
    g_StatsRequester.pitem_strings1 = g_ValidLevelStrings1;
    g_StatsRequester.pitem_strings2 = g_ValidLevelStrings2;
    g_StatsRequester.item_string_len = MAX_LEVEL_NAME_SIZE;

    Requester_Init(&g_StatsRequester);
    Requester_SetHeading(
        &g_StatsRequester, g_GF_LevelNames[g_CurrentLevel], REQ_CENTER, NULL,
        0);
    Requester_AddItem(
        &g_StatsRequester, g_GF_GameStrings[GF_S_GAME_MISC_TIME_TAKEN],
        REQ_ALIGN_LEFT, time_str, REQ_ALIGN_RIGHT);

    if (g_GF_NumSecrets) {
        char *ptr = buffer;
        int32_t num_secrets = 0;
        for (int32_t i = 0; i < 3; i++) {
            if (g_SaveGame.statistics.secrets & (1 << i)) {
                *ptr++ = 127 + i;
                num_secrets++;
            } else {
                *ptr++ = ' ';
                *ptr++ = ' ';
                *ptr++ = ' ';
            }
        }
        *ptr++ = '\0';
        if (num_secrets == 0) {
            sprintf(buffer, g_GF_GameStrings[GF_S_GAME_MISC_NONE]);
        }
        Requester_AddItem(
            &g_StatsRequester, g_GF_GameStrings[GF_S_GAME_MISC_SECRETS_FOUND],
            REQ_ALIGN_LEFT, buffer, REQ_ALIGN_RIGHT);
    }

    sprintf(buffer, "%d", g_SaveGame.statistics.kills);
    Requester_AddItem(
        &g_StatsRequester, g_GF_GameStrings[GF_S_GAME_MISC_KILLS],
        REQ_ALIGN_LEFT, buffer, REQ_ALIGN_RIGHT);

    sprintf(buffer, "%d", g_SaveGame.statistics.shots);
    Requester_AddItem(
        &g_StatsRequester, g_GF_GameStrings[GF_S_GAME_MISC_AMMO_USED],
        REQ_ALIGN_LEFT, buffer, REQ_ALIGN_RIGHT);

    sprintf(buffer, "%d", g_SaveGame.statistics.hits);
    Requester_AddItem(
        &g_StatsRequester, g_GF_GameStrings[GF_S_GAME_MISC_HITS],
        REQ_ALIGN_LEFT, buffer, REQ_ALIGN_RIGHT);

    if ((g_SaveGame.statistics.medipacks & 1) != 0) {
        sprintf(buffer, "%d.5", g_SaveGame.statistics.medipacks >> 1);
    } else {
        sprintf(buffer, "%d.0", g_SaveGame.statistics.medipacks >> 1);
    }
    Requester_AddItem(
        &g_StatsRequester, g_GF_GameStrings[GF_S_GAME_MISC_HEALTH_PACKS_USED],
        REQ_ALIGN_LEFT, buffer, REQ_ALIGN_RIGHT);

    const int32_t distance = g_SaveGame.statistics.distance / 445;
    if (distance < 1000) {
        sprintf(buffer, "%dm", distance);
    } else {
        sprintf(buffer, "%d.%02dkm", distance / 1000, distance % 100);
    }
    Requester_AddItem(
        &g_StatsRequester, g_GF_GameStrings[GF_S_GAME_MISC_DISTANCE_TRAVELLED],
        REQ_ALIGN_LEFT, buffer, REQ_ALIGN_RIGHT);

    g_StatsRequester.ready = 1;
}

void __cdecl ShowEndStatsText(void)
{
    char buffer[32];

    if (g_StatsRequester.ready) {
        if (!Requester_Display(&g_StatsRequester, 0, 1)) {
            g_InputDB = 0;
            g_Input = 0;
        }
        return;
    }

    g_StatsRequester.no_selector = 1;
    Requester_SetSize(&g_StatsRequester, 7, -32);
    g_StatsRequester.line_height = 18;
    g_StatsRequester.items_count = 0;
    g_StatsRequester.selected = 0;
    g_StatsRequester.line_offset = 0;
    g_StatsRequester.line_old_offset = 0;
    g_StatsRequester.pix_width = 304;
    g_StatsRequester.x_pos = 0;
    g_StatsRequester.z_pos = 0;
    g_StatsRequester.pitem_strings1 = g_ValidLevelStrings1;
    g_StatsRequester.pitem_strings2 = g_ValidLevelStrings2;
    g_StatsRequester.item_string_len = MAX_LEVEL_NAME_SIZE;

    Requester_Init(&g_StatsRequester);
    Requester_SetHeading(
        &g_StatsRequester, g_GF_GameStrings[GF_S_GAME_MISC_FINAL_STATISTICS], 0,
        0, 0);

    const STATS stats = M_GetEndGameStats();

    const int32_t sec = stats.timer / FRAMES_PER_SECOND;
    sprintf(
        buffer, "%02d:%02d:%02d", (sec / 60) / 60, (sec / 60) % 60, sec % 60);
    Requester_AddItem(
        &g_StatsRequester, g_GF_GameStrings[GF_S_GAME_MISC_TIME_TAKEN],
        REQ_ALIGN_LEFT, buffer, REQ_ALIGN_RIGHT);

    sprintf(
        buffer, "%d %s %d", stats.found_secrets,
        g_GF_GameStrings[GF_S_GAME_MISC_OF], stats.total_secrets);
    Requester_AddItem(
        &g_StatsRequester, g_GF_GameStrings[GF_S_GAME_MISC_SECRETS_FOUND],
        REQ_ALIGN_LEFT, buffer, REQ_ALIGN_RIGHT);

    sprintf(buffer, "%d", stats.kills);
    Requester_AddItem(
        &g_StatsRequester, g_GF_GameStrings[GF_S_GAME_MISC_KILLS],
        REQ_ALIGN_LEFT, buffer, REQ_ALIGN_RIGHT);

    sprintf(buffer, "%d", stats.ammo_used);
    Requester_AddItem(
        &g_StatsRequester, g_GF_GameStrings[GF_S_GAME_MISC_AMMO_USED],
        REQ_ALIGN_LEFT, buffer, REQ_ALIGN_RIGHT);

    sprintf(buffer, "%d", stats.ammo_hits);
    Requester_AddItem(
        &g_StatsRequester, g_GF_GameStrings[GF_S_GAME_MISC_HITS],
        REQ_ALIGN_LEFT, buffer, REQ_ALIGN_RIGHT);

    if ((stats.medipacks & 1) != 0) {
        sprintf(buffer, "%d.5", stats.medipacks >> 1);
    } else {
        sprintf(buffer, "%d.0", stats.medipacks >> 1);
    }
    Requester_AddItem(
        &g_StatsRequester, g_GF_GameStrings[GF_S_GAME_MISC_HEALTH_PACKS_USED],
        REQ_ALIGN_LEFT, buffer, REQ_ALIGN_RIGHT);

    const int32_t distance = stats.distance / 445;
    if (distance < 1000) {
        sprintf(buffer, "%dm", distance);
    } else {
        sprintf(buffer, "%d.%02dkm", distance / 1000, distance % 100);
    }
    Requester_AddItem(
        &g_StatsRequester, g_GF_GameStrings[GF_S_GAME_MISC_DISTANCE_TRAVELLED],
        REQ_ALIGN_LEFT, buffer, REQ_ALIGN_RIGHT);

    g_StatsRequester.ready = 1;
}

int32_t __cdecl LevelStats(const int32_t level_num)
{
    START_INFO *const start = &g_SaveGame.start[level_num];
    start->statistics.timer = g_SaveGame.statistics.timer;
    start->statistics.shots = g_SaveGame.statistics.shots;
    start->statistics.hits = g_SaveGame.statistics.hits;
    start->statistics.distance = g_SaveGame.statistics.distance;
    start->statistics.kills = g_SaveGame.statistics.kills;
    start->statistics.secrets = g_SaveGame.statistics.secrets;
    start->statistics.medipacks = g_SaveGame.statistics.medipacks;

    const int32_t sec = g_SaveGame.statistics.timer / FRAMES_PER_SECOND;
    char buffer[100];
    sprintf(buffer, "%02d:%02d:%02d", sec / 3600, (sec / 60) % 60, sec % 60);

    Music_Play(g_GameFlow.level_complete_track, false);

    TempVideoAdjust(g_HiRes, 1.0);
    FadeToPal(30, g_GamePalette8);
    Overlay_HideGameInfo();
    S_CopyScreenToBuffer();

    while (g_Input & IN_SELECT) {
        Input_Update();
    }

    while (true) {
        S_InitialisePolyList(0);
        S_CopyBufferToScreen();

        Input_Update();

        if (g_GF_OverrideDir != (GAME_FLOW_DIR)-1) {
            break;
        }

        ShowStatsText(buffer, 0);
        Text_Draw();
        S_OutputPolyList();
        S_DumpScreen();

        if (g_InputDB & IN_SELECT) {
            break;
        }
    }

    Requester_Shutdown(&g_StatsRequester);

    CreateStartInfo(level_num + 1);
    g_SaveGame.current_level = level_num + 1;
    start->available = 0;
    S_FadeToBlack();
    TempVideoRemove();
    return 0;
}

int32_t __cdecl GameStats(const int32_t level_num)
{
    START_INFO *const start = &g_SaveGame.start[level_num];
    start->statistics.timer = g_SaveGame.statistics.timer;
    start->statistics.shots = g_SaveGame.statistics.shots;
    start->statistics.hits = g_SaveGame.statistics.hits;
    start->statistics.distance = g_SaveGame.statistics.distance;
    start->statistics.kills = g_SaveGame.statistics.kills;
    start->statistics.secrets = g_SaveGame.statistics.secrets;
    start->statistics.medipacks = g_SaveGame.statistics.medipacks;

    Overlay_HideGameInfo();
    while (g_Input & IN_SELECT) {
        Input_Update();
    }

    while (true) {
        S_InitialisePolyList(0);
        S_CopyBufferToScreen();

        Input_Update();

        if (g_GF_OverrideDir != (GAME_FLOW_DIR)-1) {
            break;
        }

        ShowEndStatsText();
        Text_Draw();
        S_OutputPolyList();
        S_DumpScreen();

        if (g_InputDB & IN_SELECT) {
            break;
        }
    }

    Requester_Shutdown(&g_StatsRequester);

    g_SaveGame.bonus_flag = 1;
    for (int32_t level = LV_FIRST; level <= g_GameFlow.num_levels; level++) {
        ModifyStartInfo(level);
    }
    g_SaveGame.current_level = LV_FIRST;

    S_DontDisplayPicture();
    return 0;
}

int32_t __cdecl AddAssaultTime(uint32_t time)
{
    ASSAULT_STATS *const stats = &g_Assault;

    int32_t insert_idx = -1;
    for (int32_t i = 0; i < MAX_ASSAULT_TIMES; i++) {
        if (stats->best_time[i] == 0 || time < stats->best_time[i]) {
            insert_idx = i;
            break;
        }
    }
    if (insert_idx == -1) {
        return false;
    }

    for (int32_t i = MAX_ASSAULT_TIMES - 1; i > insert_idx; i--) {
        stats->best_finish[i] = stats->best_finish[i - 1];
        stats->best_time[i] = stats->best_time[i - 1];
    }

    stats->finish_count++;
    stats->best_time[insert_idx] = time;
    stats->best_finish[insert_idx] = stats->finish_count;
    return true;
}
