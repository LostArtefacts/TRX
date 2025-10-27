#include "game/ui/dialogs/stats.h"

#include "game/game_string.h"
#include "game/savegame.h"

#include <libtrx/config.h>
#include <libtrx/game/const.h>
#include <libtrx/game/game_flow.h>
#include <libtrx/game/gym.h>
#include <libtrx/game/stats.h>
#include <libtrx/game/ui.h>
#include <libtrx/strings.h>

#include <stdio.h>
#include <string.h>

typedef enum {
    M_ROW_LEVEL_COUNTER,
    M_ROW_KILLS,
    M_ROW_PICKUPS,
    M_ROW_SECRETS,
    M_ROW_DEATHS,
    M_ROW_TIMER,
    M_ROW_AMMO,
    M_ROW_MEDIPACKS_USED,
    M_ROW_DISTANCE_TRAVELLED,
} M_ROW_ROLE;

static const char *M_FormatTime(const int32_t total_frames)
{
    const int32_t total_seconds = total_frames / LOGIC_FPS;
    const int32_t hours = total_seconds / 3600;
    const int32_t minutes = (total_seconds / 60) % 60;
    const int32_t seconds = total_seconds % 60;
    if (hours != 0) {
        return String_FormatStatic("%d:%02d:%02d", hours, minutes, seconds);
    } else {
        return String_FormatStatic("%d:%02d", minutes, seconds);
    }
}

static const char *M_FormatDistance(int32_t distance)
{
    distance /= 445;
    if (distance < 1000) {
        return String_FormatStatic("%dm", distance);
    } else {
        return String_FormatStatic(
            "%d.%02dkm", distance / 1000, (distance % 1000) / 10);
    }
}

static void M_Row(
    const UI_STATS_DIALOG_STATE *const s, const char *const key,
    const char *const value)
{
    if (s->args.style == UI_STATS_DIALOG_STYLE_BARE) {
        UI_BeginStack(UI_STACK_HORIZONTAL);
        UI_Label(key);
        UI_Label(" ");
        UI_Label(value);
        UI_EndStack();
    } else {
        UI_BeginStackEx((UI_STACK_SETTINGS) {
            .orientation = UI_STACK_HORIZONTAL,
            .spacing = { .h = 30.0f },
            .align = { .h = UI_STACK_H_ALIGN_DISTRIBUTE },
        });
        UI_Label(key);
        UI_Label(value);
        UI_EndStack();
    }
}

static void M_RowFromRole(
    const UI_STATS_DIALOG_STATE *const s, const M_ROW_ROLE role,
    const STATS_COMMON *const stats, const LEVEL_MAX_STATS *const max_stats)
{
    const char *const num_fmt = g_Config.ui.stat_detail_mode == SDM_MINIMAL
        ? GS(STATS_BASIC_FMT)
        : GS(STATS_DETAIL_FMT);

    switch (role) {
    case M_ROW_LEVEL_COUNTER:
        M_Row(
            s, GS(STATS_LEVEL),
            String_FormatStatic(
                GS(STATS_DETAIL_FMT),
                GF_GetLevelOrdinalNumber(
                    GFLT_MAIN, GF_GetLevel(GFLT_MAIN, s->args.level_num)),
                GF_GetLevelCount(GFLT_MAIN)));
        break;

    case M_ROW_TIMER:
        M_Row(s, GS(STATS_TIME_TAKEN), M_FormatTime(stats->timer));
        break;

    case M_ROW_SECRETS:
        M_Row(
            s, GS(STATS_SECRETS),
            String_FormatStatic(
                GS(STATS_DETAIL_FMT), stats->secret_count,
                max_stats->max_secret_count));
        break;

    case M_ROW_PICKUPS:
        M_Row(
            s, GS(STATS_PICKUPS),
            String_FormatStatic(
                num_fmt, stats->pickup_count, max_stats->max_pickup_count));
        break;

    case M_ROW_KILLS:
        M_Row(
            s, GS(STATS_KILLS),
            String_FormatStatic(
                num_fmt, stats->kill_count, max_stats->max_kill_count));
        break;

    case M_ROW_DEATHS:
        M_Row(
            s, GS(STATS_DEATHS),
            String_FormatStatic(GS(STATS_BASIC_FMT), stats->death_count));
        break;

    case M_ROW_AMMO:
        M_Row(
            s, GS(STATS_AMMO),
            String_FormatStatic(
                GS(PAGINATION_NAV), stats->ammo_hits, stats->ammo_used));
        break;

    case M_ROW_MEDIPACKS_USED:
        M_Row(
            s, GS(STATS_MEDIPACKS_USED),
            String_FormatStatic("%.1f", stats->medipacks_used));
        break;

    case M_ROW_DISTANCE_TRAVELLED:
        M_Row(
            s, GS(STATS_DISTANCE_TRAVELLED),
            M_FormatDistance(stats->distance_travelled));
        break;

    default:
        break;
    }
}

static void M_CommonRows(
    const UI_STATS_DIALOG_STATE *const s, const STATS_COMMON *const stats,
    const LEVEL_MAX_STATS *const max_stats)
{
    if (g_Config.ui.stat_detail_mode == SDM_MINIMAL) {
        M_RowFromRole(s, M_ROW_KILLS, stats, max_stats);
        M_RowFromRole(s, M_ROW_PICKUPS, stats, max_stats);
        M_RowFromRole(s, M_ROW_SECRETS, stats, max_stats);
        M_RowFromRole(s, M_ROW_TIMER, stats, max_stats);
    } else {
        M_RowFromRole(s, M_ROW_TIMER, stats, max_stats);
        M_RowFromRole(s, M_ROW_SECRETS, stats, max_stats);
        M_RowFromRole(s, M_ROW_PICKUPS, stats, max_stats);
        M_RowFromRole(s, M_ROW_KILLS, stats, max_stats);
        if (g_Config.ui.stat_detail_mode == SDM_FULL) {
            M_RowFromRole(s, M_ROW_AMMO, stats, max_stats);
            M_RowFromRole(s, M_ROW_MEDIPACKS_USED, stats, max_stats);
            M_RowFromRole(s, M_ROW_DISTANCE_TRAVELLED, stats, max_stats);
        }
    }

    if (g_Config.gameplay.enable_deaths_counter && stats->death_count >= 0) {
        // Always use sum of all levels for the deaths.
        // Deaths get stored in the resume info for the level they happen
        // on, so if the player dies in Vilcabamba and reloads Caves, they
        // should still see an incremented death counter.
        M_RowFromRole(s, M_ROW_DEATHS, stats, max_stats);
    }
}

static void M_LevelStatsRows(const UI_STATS_DIALOG_STATE *const s)
{
    const GF_LEVEL *const current_level =
        GF_GetLevel(GFLT_MAIN, s->args.level_num);
    const RESUME_INFO *const resume = Savegame_GetCurrentInfo(current_level);
    const STATS_COMMON *const stats = (STATS_COMMON *)&resume->stats;
    const LEVEL_MAX_STATS *const max_stats = &resume->max_stats;
    if (g_Config.ui.enable_stats_level_header) {
        M_RowFromRole(s, M_ROW_LEVEL_COUNTER, stats, max_stats);
    }
    M_CommonRows(s, stats, max_stats);
}

static void M_FinalStatsRows(const UI_STATS_DIALOG_STATE *const s)
{
    const GF_LEVEL_TYPE level_type =
        GF_GetLevel(GFLT_MAIN, s->args.level_num)->type;
    const FINAL_STATS final_stats = Stats_ComputeFinalStats(level_type);
    const STATS_COMMON *const stats = &final_stats.stats;
    const LEVEL_MAX_STATS *const max_stats = &final_stats.max_stats;
    M_CommonRows(s, stats, max_stats);
}

static const char *M_GetDialogTitle(const UI_STATS_DIALOG_STATE *const s)
{
    switch (s->args.mode) {
    case UI_STATS_DIALOG_MODE_LEVEL:
        return GF_GetLevel(GFLT_MAIN, s->args.level_num)->title;
    case UI_STATS_DIALOG_MODE_FINAL: {
        const GF_LEVEL_TYPE level_type =
            GF_GetLevel(GFLT_MAIN, s->args.level_num)->type;
        const char *const title = level_type == GFL_BONUS
            ? GS(STATS_BONUS_STATISTICS)
            : GS(STATS_FINAL_STATISTICS);
        return title;
    }
    case UI_STATS_DIALOG_MODE_ASSAULT_COURSE:
        return GS(STATS_ASSAULT_TITLE);
    }
    return nullptr;
}

static void M_AssaultCourseStatsRows(UI_STATS_DIALOG_STATE *const s)
{
    const ASSAULT_STATS stats = Gym_GetAssaultStats();
    UI_BeginRequester(&s->assault_req, M_GetDialogTitle(s));
    // ensure minimum dialog width
    UI_Spacer(290.0f, 0.0f);
    if (stats.entries[0].time == 0) {
        UI_BeginAnchor(0.5f, 0.5f);
        UI_Label(GS(STATS_ASSAULT_NO_TIMES_SET));
        UI_EndAnchor();
    } else {
        const int32_t first = UI_Requester_GetFirstRow(&s->assault_req);
        const int32_t last = UI_Requester_GetLastRow(&s->assault_req);
        for (int32_t i = first; i < last; i++) {
            if (stats.entries[i].time == 0) {
                break;
            }

            char left_buf[32] = " ";
            char right_buf[32] = " ";
            sprintf(
                left_buf, "%2d: %s %d", i + 1, GS(STATS_ASSAULT_FINISH),
                stats.entries[i].attempt_num);

            const int32_t sec = stats.entries[i].time / LOGIC_FPS;
            sprintf(
                right_buf, "%02d:%02d.%-2d", sec / 60, sec % 60,
                stats.entries[i].time % LOGIC_FPS / (LOGIC_FPS / 10));

            M_Row(s, left_buf, right_buf);
        }
    }
    UI_EndRequester(&s->assault_req);
}

static void M_BeginDialog(const UI_STATS_DIALOG_STATE *const s)
{
    const char *const title = M_GetDialogTitle(s);
    UI_BeginModal(0.5f, 0.5f);
    if (s->args.style == UI_STATS_DIALOG_STYLE_BARE) {
        UI_BeginStackEx((UI_STACK_SETTINGS) {
            .orientation = UI_STACK_VERTICAL,
            .spacing = { .v = 11.0f },
            .align = { .h = UI_STACK_H_ALIGN_CENTER },
        });
        if (title != nullptr) {
            UI_Label(title);
        }
    } else {
        UI_BeginWindow();
        if (title != nullptr) {
            UI_WindowTitle(title);
        }
        UI_BeginWindowBody();
        UI_BeginStackEx((UI_STACK_SETTINGS) {
            .orientation = UI_STACK_VERTICAL,
            .spacing = { .v = 4.0f },
            .align = { .h = UI_STACK_H_ALIGN_SPAN },
        });
    }
}

static void M_EndDialog(const UI_STATS_DIALOG_STATE *const s)
{
    if (s->args.style == UI_STATS_DIALOG_STYLE_BARE) {
        UI_EndStack();
    } else {
        UI_EndStack();
        UI_EndWindowBody();
        UI_EndWindow();
    }
    UI_EndModal();
}

void UI_StatsDialog(UI_STATS_DIALOG_STATE *const s)
{
    M_BeginDialog(s);

    switch (s->args.mode) {
    case UI_STATS_DIALOG_MODE_LEVEL:
        M_LevelStatsRows(s);
        break;
    case UI_STATS_DIALOG_MODE_FINAL:
        M_FinalStatsRows(s);
        break;
    case UI_STATS_DIALOG_MODE_ASSAULT_COURSE:
        M_AssaultCourseStatsRows(s);
        break;
    }

    M_EndDialog(s);
}
