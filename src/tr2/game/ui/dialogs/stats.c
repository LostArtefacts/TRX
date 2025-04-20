#include "game/ui/dialogs/stats.h"

#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/savegame.h"
#include "game/stats.h"

#include <libtrx/debug.h>
#include <libtrx/game/gym.h>
#include <libtrx/game/ui/common.h>
#include <libtrx/game/ui/elements/anchor.h>
#include <libtrx/game/ui/elements/label.h>
#include <libtrx/game/ui/elements/modal.h>
#include <libtrx/game/ui/elements/pad.h>
#include <libtrx/game/ui/elements/requester.h>
#include <libtrx/game/ui/elements/resize.h>
#include <libtrx/game/ui/elements/spacer.h>
#include <libtrx/game/ui/elements/stack.h>
#include <libtrx/game/ui/elements/window.h>

#include <stdio.h>
#include <string.h>

typedef enum {
    M_ROW_GENERIC,
    M_ROW_TIMER,
    M_ROW_LEVEL_SECRETS,
    M_ROW_ALL_SECRETS,
    M_ROW_KILLS,
    M_ROW_AMMO_USED,
    M_ROW_AMMO_HITS,
    M_ROW_MEDIPACKS,
    M_ROW_DISTANCE_TRAVELLED,
} M_ROW_ROLE;

static void M_FormatTime(char *out, int32_t total_frames);
static void M_FormatSecrets(char *out, const LEVEL_STATS *level_stats);
static void M_FormatDistance(char *const out, int32_t distance);
static void M_Row(
    const UI_STATS_DIALOG_STATE *s, const char *key, const char *value);
static void M_RowFromRole(
    const UI_STATS_DIALOG_STATE *s, M_ROW_ROLE role, const STATS_COMMON *stats);
static void M_LevelStatsRows(const UI_STATS_DIALOG_STATE *s);
static void M_FinalStatsRows(const UI_STATS_DIALOG_STATE *s);
static void M_AssaultCourseStatsRows(UI_STATS_DIALOG_STATE *s);
static const char *M_GetDialogTitle(const UI_STATS_DIALOG_STATE *s);
static void M_BeginDialog(const UI_STATS_DIALOG_STATE *s);
static void M_EndDialog(const UI_STATS_DIALOG_STATE *s);

static void M_FormatTime(char *const out, const int32_t total_frames)
{
    const int32_t total_seconds = total_frames / LOGIC_FPS;
    const int32_t hours = total_seconds / 3600;
    const int32_t minutes = (total_seconds / 60) % 60;
    const int32_t seconds = total_seconds % 60;
    sprintf(out, "%02d:%02d:%02d", hours, minutes, seconds);
}

static void M_FormatSecrets(
    char *const out, const LEVEL_STATS *const level_stats)
{
    char *ptr = out;
    int32_t num_secrets = 0;
    for (int32_t i = 1; i >= 0; i--) {
        for (int32_t j = 0; j < 3; j++) {
            const int32_t flag = 1 << (j + i * 3);
            if ((level_stats->secret_flags & flag) != 0) {
                sprintf(ptr, "\\{secret %d}", j + 1);
                num_secrets++;
            } else {
                strcpy(ptr, "   ");
            }
            ptr += strlen(ptr);
        }
    }

    *ptr++ = '\0';
    if (num_secrets == 0) {
        strcpy(out, GS(MISC_NONE));
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
    UI_BeginStackEx((UI_STACK_SETTINGS) {
        .orientation = UI_STACK_HORIZONTAL,
        .spacing = { .h = TR_VERSION == 1 ? 30.0f : 90.0f },
        .align = { .h = UI_STACK_H_ALIGN_DISTRIBUTE },
    });
    UI_Label(key);
    UI_Label(value);
    UI_EndStack();
}

static void M_RowFromRole(
    const UI_STATS_DIALOG_STATE *const s, const M_ROW_ROLE role,
    const STATS_COMMON *const stats)
{
    char buf[64];

    switch (role) {
    case M_ROW_TIMER: {
        M_FormatTime(buf, stats->timer);
        M_Row(s, GS(STATS_TIME_TAKEN), buf);
        break;
    }

    case M_ROW_LEVEL_SECRETS: {
        M_FormatSecrets(buf, (LEVEL_STATS *)stats);
        M_Row(s, GS(STATS_SECRETS), buf);
        break;
    }

    case M_ROW_ALL_SECRETS:
        sprintf(
            buf, GS(STATS_DETAIL_FMT), ((FINAL_STATS *)stats)->found_secrets,
            ((FINAL_STATS *)stats)->total_secrets);
        M_Row(s, GS(STATS_SECRETS), buf);
        break;

    case M_ROW_KILLS:
        sprintf(buf, GS(STATS_BASIC_FMT), stats->kill_count);
        M_Row(s, GS(STATS_KILLS), buf);
        break;

    case M_ROW_AMMO_USED:
        sprintf(buf, "%d", stats->ammo_used);
        M_Row(s, GS(STATS_AMMO_USED), buf);
        break;

    case M_ROW_AMMO_HITS:
        sprintf(buf, "%d", stats->ammo_hits);
        M_Row(s, GS(STATS_AMMO_HITS), buf);
        break;

    case M_ROW_MEDIPACKS:
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

static void M_LevelStatsRows(const UI_STATS_DIALOG_STATE *const s)
{
    const GF_LEVEL *const current_level =
        GF_GetLevel(GFLT_MAIN, s->args.level_num);
    const RESUME_INFO *const current_info =
        Savegame_GetCurrentInfo(current_level);
    const STATS_COMMON *const stats = (STATS_COMMON *)&current_info->stats;

    M_RowFromRole(s, M_ROW_TIMER, stats);
    if (stats->max_secret_count != 0) {
        M_RowFromRole(s, M_ROW_LEVEL_SECRETS, stats);
    }
    M_RowFromRole(s, M_ROW_KILLS, stats);
    M_RowFromRole(s, M_ROW_AMMO_USED, stats);
    M_RowFromRole(s, M_ROW_AMMO_HITS, stats);
    M_RowFromRole(s, M_ROW_MEDIPACKS, stats);
    M_RowFromRole(s, M_ROW_DISTANCE_TRAVELLED, stats);
}

static void M_FinalStatsRows(const UI_STATS_DIALOG_STATE *const s)
{
    const GF_LEVEL_TYPE level_type =
        GF_GetLevel(GFLT_MAIN, s->args.level_num)->type;
    const FINAL_STATS final_stats = Stats_ComputeFinalStats(level_type);
    const STATS_COMMON *const stats = (STATS_COMMON *)&final_stats;
    M_RowFromRole(s, M_ROW_TIMER, stats);
    M_RowFromRole(s, M_ROW_ALL_SECRETS, stats);
    M_RowFromRole(s, M_ROW_KILLS, stats);
    M_RowFromRole(s, M_ROW_AMMO_USED, stats);
    M_RowFromRole(s, M_ROW_AMMO_HITS, stats);
    M_RowFromRole(s, M_ROW_MEDIPACKS, stats);
    M_RowFromRole(s, M_ROW_DISTANCE_TRAVELLED, stats);
}

static void M_AssaultCourseStatsRows(UI_STATS_DIALOG_STATE *const s)
{
    const ASSAULT_STATS stats = Gym_GetAssaultStats();
    UI_BeginRequester(&s->assault_req, M_GetDialogTitle(s));
    // ensure minimum dialog width
    UI_Spacer(290.0f, 0.0f);
    if (stats.best_time[0] == 0) {
        UI_BeginAnchor(0.5f, 0.5f);
        UI_Label(GS(STATS_ASSAULT_NO_TIMES_SET));
        UI_EndAnchor();
    } else {
        const int32_t first = UI_Requester_GetFirstRow(&s->assault_req);
        const int32_t last = UI_Requester_GetLastRow(&s->assault_req);
        for (int32_t i = first; i < last; i++) {
            if (stats.best_time[i] == 0) {
                break;
            }

            char left_buf[32] = " ";
            char right_buf[32] = " ";
            sprintf(
                left_buf, "%2d: %s %d", i + 1, GS(STATS_ASSAULT_FINISH),
                stats.best_finish[i]);

            const int32_t sec = stats.best_time[i] / FRAMES_PER_SECOND;
            sprintf(
                right_buf, "%02d:%02d.%-2d", sec / 60, sec % 60,
                stats.best_time[i] % FRAMES_PER_SECOND
                    / (FRAMES_PER_SECOND / 10));

            M_Row(s, left_buf, right_buf);
        }
    }
    UI_EndRequester(&s->assault_req);
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

static void M_BeginDialog(const UI_STATS_DIALOG_STATE *const s)
{
    const char *const title = M_GetDialogTitle(s);
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
            .spacing = { .v = 3.0f },
            .align = { .h = UI_STACK_H_ALIGN_SPAN },
        });
    }
    // ensure minimum dialog width
    UI_Spacer(290.0f, 0.0f);
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
}

void UI_StatsDialog(UI_STATS_DIALOG_STATE *const s)
{
    // TODO: add support for the bare style by merging TR1 and TR2 stats dialog
    // implementations.
    ASSERT(s->args.style == UI_STATS_DIALOG_STYLE_BORDERED);

    UI_BeginModal(0.5f, 1.0f);
    UI_BeginPad(40.f, 40.0f);

    switch (s->args.mode) {
    case UI_STATS_DIALOG_MODE_LEVEL:
        M_BeginDialog(s);
        M_LevelStatsRows(s);
        M_EndDialog(s);
        break;
    case UI_STATS_DIALOG_MODE_FINAL:
        M_BeginDialog(s);
        M_FinalStatsRows(s);
        M_EndDialog(s);
        break;
    case UI_STATS_DIALOG_MODE_ASSAULT_COURSE:
        M_AssaultCourseStatsRows(s);
        break;
    }

    UI_EndPad();
    UI_EndModal();
}
