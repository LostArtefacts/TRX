#include "game/ui/dialogs/stats.h"

#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/savegame.h"
#include "game/stats.h"

#include <libtrx/config.h>
#include <libtrx/game/ui/common.h>
#include <libtrx/game/ui/elements/anchor.h>
#include <libtrx/game/ui/elements/frame.h>
#include <libtrx/game/ui/elements/label.h>
#include <libtrx/game/ui/elements/modal.h>
#include <libtrx/game/ui/elements/stack.h>
#include <libtrx/game/ui/elements/window.h>
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

static void M_FormatTime(char *const out, const int32_t total_frames)
{
    const int32_t total_seconds = total_frames / LOGIC_FPS;
    const int32_t hours = total_seconds / 3600;
    const int32_t minutes = (total_seconds / 60) % 60;
    const int32_t seconds = total_seconds % 60;
    if (hours != 0) {
        sprintf(out, "%d:%02d:%02d", hours, minutes, seconds);
    } else {
        sprintf(out, "%d:%02d", minutes, seconds);
    }
}

static void M_FormatDistance(char *const out, int32_t distance)
{
    distance /= 445;
    if (distance < 1000) {
        sprintf(out, "%dm", distance);
    } else {
        sprintf(out, "%d.%02dkm", distance / 1000, (distance % 1000) / 10);
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
    const STATS_COMMON *const stats)
{
    char buf[50];
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

    case M_ROW_KILLS:
        sprintf(buf, num_fmt, stats->kill_count, stats->max_kill_count);
        M_Row(s, GS(STATS_KILLS), buf);
        break;

    case M_ROW_PICKUPS:
        sprintf(buf, num_fmt, stats->pickup_count, stats->max_pickup_count);
        M_Row(s, GS(STATS_PICKUPS), buf);
        break;

    case M_ROW_SECRETS:
        sprintf(
            buf, GS(STATS_DETAIL_FMT), stats->secret_count,
            stats->max_secret_count);
        M_Row(s, GS(STATS_SECRETS), buf);
        break;

    case M_ROW_DEATHS:
        sprintf(buf, GS(STATS_BASIC_FMT), stats->death_count);
        M_Row(s, GS(STATS_DEATHS), buf);
        break;

    case M_ROW_TIMER:
        M_FormatTime(buf, stats->timer);
        M_Row(s, GS(STATS_TIME_TAKEN), buf);
        break;

    case M_ROW_AMMO:
        sprintf(buf, GS(PAGINATION_NAV), stats->ammo_hits, stats->ammo_used);
        M_Row(s, GS(STATS_AMMO), buf);
        break;

    case M_ROW_MEDIPACKS_USED:
        sprintf(buf, "%.1f", stats->medipacks_used);
        M_Row(s, GS(STATS_MEDIPACKS_USED), buf);
        break;

    case M_ROW_DISTANCE_TRAVELLED:
        M_FormatDistance(buf, stats->distance_travelled);
        M_Row(s, GS(STATS_DISTANCE_TRAVELLED), buf);
        break;

    default:
        break;
    }
}

static void M_CommonRows(
    const UI_STATS_DIALOG_STATE *const s, const STATS_COMMON *const stats)
{
    if (g_Config.ui.stat_detail_mode == SDM_MINIMAL) {
        M_RowFromRole(s, M_ROW_KILLS, stats);
        M_RowFromRole(s, M_ROW_PICKUPS, stats);
        M_RowFromRole(s, M_ROW_SECRETS, stats);
        M_RowFromRole(s, M_ROW_TIMER, stats);
    } else {
        M_RowFromRole(s, M_ROW_TIMER, stats);
        M_RowFromRole(s, M_ROW_SECRETS, stats);
        M_RowFromRole(s, M_ROW_PICKUPS, stats);
        M_RowFromRole(s, M_ROW_KILLS, stats);
        if (g_Config.ui.stat_detail_mode == SDM_FULL) {
            M_RowFromRole(s, M_ROW_AMMO, stats);
            M_RowFromRole(s, M_ROW_MEDIPACKS_USED, stats);
            M_RowFromRole(s, M_ROW_DISTANCE_TRAVELLED, stats);
        }
    }

    if (g_Config.gameplay.enable_deaths_counter && stats->death_count >= 0) {
        // Always use sum of all levels for the deaths.
        // Deaths get stored in the resume info for the level they happen
        // on, so if the player dies in Vilcabamba and reloads Caves, they
        // should still see an incremented death counter.
        M_RowFromRole(s, M_ROW_DEATHS, stats);
    }
}

static void M_LevelStatsRows(const UI_STATS_DIALOG_STATE *const s)
{
    const GF_LEVEL *const current_level =
        GF_GetLevel(GFLT_MAIN, s->args.level_num);
    const RESUME_INFO *const current_info =
        Savegame_GetCurrentInfo(current_level);
    const STATS_COMMON *const stats = (STATS_COMMON *)&current_info->stats;
    if (g_Config.ui.enable_stats_level_header) {
        M_RowFromRole(s, M_ROW_LEVEL_COUNTER, stats);
    }
    M_CommonRows(s, stats);
}

static void M_FinalStatsRows(const UI_STATS_DIALOG_STATE *const s)
{
    FINAL_STATS final_stats;
    const GF_LEVEL_TYPE level_type =
        GF_GetLevel(GFLT_MAIN, s->args.level_num)->type;
    Stats_ComputeFinal(level_type, &final_stats);
    M_CommonRows(s, (STATS_COMMON *)&final_stats);
}

static const char *M_GetDialogTitle(const UI_STATS_DIALOG_STATE *const s)
{
    switch (s->args.mode) {
    case UI_STATS_DIALOG_MODE_LEVEL:
        return GF_GetLevel(GFLT_MAIN, s->args.level_num)->title;

    case UI_STATS_DIALOG_MODE_FINAL: {
        const GF_LEVEL_TYPE level_type =
            GF_GetLevel(GFLT_MAIN, s->args.level_num)->type;
        if (level_type == GFL_BONUS) {
            return GS(STATS_BONUS_STATISTICS);
        }
        return GS(STATS_FINAL_STATISTICS);
    }
    }

    return nullptr;
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
    }

    M_EndDialog(s);
}
