#include "game/ui2/dialogs/stats.h"

#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/savegame.h"
#include "game/stats.h"

#include <libtrx/config.h>
#include <libtrx/game/ui2/common.h>
#include <libtrx/game/ui2/elements/anchor.h>
#include <libtrx/game/ui2/elements/frame.h>
#include <libtrx/game/ui2/elements/label.h>
#include <libtrx/game/ui2/elements/modal.h>
#include <libtrx/game/ui2/elements/stack.h>
#include <libtrx/game/ui2/elements/window.h>

#include <stdio.h>
#include <string.h>

typedef enum {
    M_ROW_KILLS,
    M_ROW_PICKUPS,
    M_ROW_SECRETS,
    M_ROW_DEATHS,
    M_ROW_TIMER,
    M_ROW_AMMO,
    M_ROW_MEDIPACKS_USED,
    M_ROW_DISTANCE_TRAVELLED,
} M_ROW_ROLE;

static void M_FormatTime(char *out, int32_t total_frames);
static void M_FormatDistance(char *const out, int32_t distance);
static void M_Row(
    const UI2_STATS_DIALOG_STATE *s, const char *key, const char *value);
static void M_RowFromRole(
    const UI2_STATS_DIALOG_STATE *s, M_ROW_ROLE role,
    const STATS_COMMON *stats);
static void M_CommonRows(
    const UI2_STATS_DIALOG_STATE *s, const STATS_COMMON *stats);
static void M_LevelStatsRows(const UI2_STATS_DIALOG_STATE *s);
static void M_FinalStatsRows(const UI2_STATS_DIALOG_STATE *s);
static const char *M_GetDialogTitle(const UI2_STATS_DIALOG_STATE *s);
static void M_BeginDialog(const UI2_STATS_DIALOG_STATE *s);
static void M_EndDialog(const UI2_STATS_DIALOG_STATE *s);

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
    const UI2_STATS_DIALOG_STATE *const s, const char *const key,
    const char *const value)
{
    if (s->args.style == UI2_STATS_DIALOG_STYLE_BARE) {
        UI2_BeginStack(UI2_STACK_HORIZONTAL);
        UI2_Label(key);
        UI2_Label(" ");
        UI2_Label(value);
        UI2_EndStack();
    } else {
        UI2_BeginStackEx((UI2_STACK_SETTINGS) {
            .orientation = UI2_STACK_HORIZONTAL,
            .spacing = { .h = 30.0f },
            .align = { .h = UI2_STACK_H_ALIGN_DISTRIBUTE },
        });
        UI2_Label(key);
        UI2_Label(value);
        UI2_EndStack();
    }
}

static void M_RowFromRole(
    const UI2_STATS_DIALOG_STATE *const s, const M_ROW_ROLE role,
    const STATS_COMMON *const stats)
{
    char buf[50];
    const char *const num_fmt =
        g_Config.gameplay.stat_detail_mode == SDM_MINIMAL
        ? GS(STATS_BASIC_FMT)
        : GS(STATS_DETAIL_FMT);

    switch (role) {
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
        sprintf(buf, GS(DETAIL_FLOAT_FMT), stats->medipacks_used);
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
    const UI2_STATS_DIALOG_STATE *const s, const STATS_COMMON *const stats)
{
    if (g_Config.gameplay.stat_detail_mode == SDM_MINIMAL) {
        M_RowFromRole(s, M_ROW_KILLS, stats);
        M_RowFromRole(s, M_ROW_PICKUPS, stats);
        M_RowFromRole(s, M_ROW_SECRETS, stats);
        M_RowFromRole(s, M_ROW_TIMER, stats);
    } else {
        M_RowFromRole(s, M_ROW_TIMER, stats);
        M_RowFromRole(s, M_ROW_SECRETS, stats);
        M_RowFromRole(s, M_ROW_PICKUPS, stats);
        M_RowFromRole(s, M_ROW_KILLS, stats);
        if (g_Config.gameplay.stat_detail_mode == SDM_FULL) {
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

static void M_LevelStatsRows(const UI2_STATS_DIALOG_STATE *const s)
{
    const GF_LEVEL *const current_level =
        GF_GetLevel(GFLT_MAIN, s->args.level_num);
    const RESUME_INFO *const current_info =
        Savegame_GetCurrentInfo(current_level);
    const STATS_COMMON *const stats = (STATS_COMMON *)&current_info->stats;
    M_CommonRows(s, stats);
}

static void M_FinalStatsRows(const UI2_STATS_DIALOG_STATE *const s)
{
    FINAL_STATS final_stats;
    const GF_LEVEL_TYPE level_type =
        GF_GetLevel(GFLT_MAIN, s->args.level_num)->type;
    Stats_ComputeFinal(level_type, &final_stats);
    M_CommonRows(s, (STATS_COMMON *)&final_stats);
}

static const char *M_GetDialogTitle(const UI2_STATS_DIALOG_STATE *const s)
{
    switch (s->args.mode) {
    case UI2_STATS_DIALOG_MODE_LEVEL:
        return GF_GetLevel(GFLT_MAIN, s->args.level_num)->title;

    case UI2_STATS_DIALOG_MODE_FINAL: {
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

static void M_BeginDialog(const UI2_STATS_DIALOG_STATE *const s)
{
    const char *const title = M_GetDialogTitle(s);
    UI2_BeginModal(0.5f, 0.5f);
    if (s->args.style == UI2_STATS_DIALOG_STYLE_BARE) {
        UI2_BeginStackEx((UI2_STACK_SETTINGS) {
            .orientation = UI2_STACK_VERTICAL,
            .spacing = { .v = 11.0f },
            .align = { .h = UI2_STACK_H_ALIGN_CENTER },
        });
        if (title != nullptr) {
            UI2_Label(title);
        }
    } else {
        UI2_BeginWindow();
        if (title != nullptr) {
            UI2_WindowTitle(title);
        }
        UI2_BeginWindowBody();
        UI2_BeginStackEx((UI2_STACK_SETTINGS) {
            .orientation = UI2_STACK_VERTICAL,
            .spacing = { .v = 4.0f },
            .align = { .h = UI2_STACK_H_ALIGN_SPAN },
        });
    }
}

static void M_EndDialog(const UI2_STATS_DIALOG_STATE *const s)
{
    if (s->args.style == UI2_STATS_DIALOG_STYLE_BARE) {
        UI2_EndStack();
    } else {
        UI2_EndStack();
        UI2_EndWindowBody();
        UI2_EndWindow();
    }
    UI2_EndModal();
}

void UI2_StatsDialog(UI2_STATS_DIALOG_STATE *const s)
{
    M_BeginDialog(s);

    switch (s->args.mode) {
    case UI2_STATS_DIALOG_MODE_LEVEL:
        M_LevelStatsRows(s);
        break;
    case UI2_STATS_DIALOG_MODE_FINAL:
        M_FinalStatsRows(s);
        break;
    }

    M_EndDialog(s);
}
