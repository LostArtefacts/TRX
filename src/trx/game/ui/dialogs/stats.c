#include <trx/config.h>
#include <trx/debug.h>
#include <trx/game/const.h>
#include <trx/game/game.h>
#include <trx/game/game_flow.h>
#include <trx/game/gym.h>
#include <trx/game/objects.h>
#include <trx/game/savegame.h>
#include <trx/game/stats.h>
#include <trx/game/ui.h>
#include <trx/memory.h>
#include <trx/strings.h>
#include <trx/version.h>

typedef enum {
    M_ROW_GENERIC,
    M_ROW_LEVEL_COUNTER,
    M_ROW_TIMER,
    M_ROW_ICON_SECRETS,
    M_ROW_NUM_SECRETS,
    M_ROW_PICKUPS,
    M_ROW_DEATHS,
    M_ROW_KILLS,
    M_ROW_AMMO,
    M_ROW_AMMO_USED,
    M_ROW_AMMO_HITS,
    M_ROW_MEDIPACKS_USED,
    M_ROW_DISTANCE_TRAVELLED,
} M_ROW_ROLE;

typedef struct {
    float window_margin;
    float window_y;
    float window_pad;
    float min_width;
    float row_spacing;
    bool use_full_hours;
} M_LOOK;

typedef struct UI_STATS_DIALOG_STATE {
    UI_STATS_DIALOG_ARGS args;
    UI_REQUESTER_STATE assault_req;
    const M_LOOK *look;
    bool has_floordata_secrets;
} UI_STATS_DIALOG_STATE;

static const M_LOOK m_Looks[TR_VERSION_COUNT] = {
    [0] = {
        .window_margin = 0.0f,
        .window_y = 0.5f,
        .window_pad = 4.0f,
        .min_width = 0.0f,
        .row_spacing = 30.0f,
        .use_full_hours = false,
    },
    [1] = {
        .window_margin = 40.0f,
        .window_y = 1.0f,
        .window_pad = 3.0f,
        .min_width = 290.0f,
        .row_spacing = 25.0f,
        .use_full_hours = true,
    },
    [2] = {
        .window_margin = 40.0f,
        .window_y = 1.0f,
        .window_pad = 3.0f,
        .min_width = 290.0f,
        .row_spacing = 25.0f,
        .use_full_hours = true,
    },
};

static const char *M_FormatTime(
    const UI_STATS_DIALOG_STATE *const s, const int32_t total_frames)
{
    const int32_t total_seconds = total_frames / LOGIC_FPS;
    const int32_t hours = total_seconds / 3600;
    const int32_t minutes = (total_seconds / 60) % 60;
    const int32_t seconds = total_seconds % 60;
    if (s->look->use_full_hours) {
        return String_FormatStatic("%02d:%02d:%02d", hours, minutes, seconds);
    } else if (hours != 0) {
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

static void M_FormatIconSecrets(
    char *const out, const LEVEL_STATS *const level_stats)
{
    char *ptr = out;
    int32_t num_secrets = 0;
    for (int32_t i = 0; i < STATS_MAX_SECRETS; i++) {
        if (!Stats_IsSecretValid(i)) {
            continue;
        }

        const bool has_secret = Stats_HasSecret(i);
        if (!has_secret && out == ptr) {
            // Do not reserve space pointlessly.
            // Good: [secret][ ][ ]
            // Bad:  [ ][ ][secret] – should be just [secret]
            continue;
        }
        const OBJECT_ID obj_id = Stats_GetSecretObject(i);
        if (obj_id != NO_OBJECT) {
            ptr += sprintf(
                ptr, has_secret ? "\\{secret %d}" : "\\{i}\\{secret %d}\\{/i}",
                obj_id + 1 - O_SECRET_1);
        }
        if (has_secret) {
            num_secrets++;
        }
    }
    *ptr++ = '\0';

    if (num_secrets == 0) {
        strcpy(out, GS(STATS_NONE));
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
            .spacing = { .h = s->look->row_spacing },
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
        M_Row(s, GS(STATS_TIME_TAKEN), M_FormatTime(s, stats->timer));
        break;

    case M_ROW_ICON_SECRETS: {
        if (s->has_floordata_secrets) {
            M_RowFromRole(s, M_ROW_NUM_SECRETS, stats, max_stats);
            break;
        }
        char buf[256];
        M_FormatIconSecrets(buf, (LEVEL_STATS *)stats);
        M_Row(s, GS(STATS_SECRETS), buf);
        break;
    }

    case M_ROW_NUM_SECRETS:
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

    case M_ROW_AMMO_USED:
        M_Row(
            s, GS(STATS_AMMO_USED),
            String_FormatStatic("%d", stats->ammo_used));
        break;

    case M_ROW_AMMO_HITS:
        M_Row(
            s, GS(STATS_AMMO_HITS),
            String_FormatStatic("%d", stats->ammo_hits));
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
    if (g_TRVersion == 1) {
        if (g_Config.ui.stat_detail_mode == SDM_MINIMAL) {
            M_RowFromRole(s, M_ROW_KILLS, stats, max_stats);
            M_RowFromRole(s, M_ROW_PICKUPS, stats, max_stats);
            M_RowFromRole(s, M_ROW_NUM_SECRETS, stats, max_stats);
            M_RowFromRole(s, M_ROW_TIMER, stats, max_stats);
        } else {
            M_RowFromRole(s, M_ROW_TIMER, stats, max_stats);
            M_RowFromRole(s, M_ROW_NUM_SECRETS, stats, max_stats);
            M_RowFromRole(s, M_ROW_PICKUPS, stats, max_stats);
            M_RowFromRole(s, M_ROW_KILLS, stats, max_stats);
            if (g_Config.ui.stat_detail_mode == SDM_FULL) {
                M_RowFromRole(s, M_ROW_AMMO, stats, max_stats);
                M_RowFromRole(s, M_ROW_MEDIPACKS_USED, stats, max_stats);
                M_RowFromRole(s, M_ROW_DISTANCE_TRAVELLED, stats, max_stats);
            }
        }
    } else {
        if (g_Config.ui.stat_detail_mode == SDM_FULL) {
            M_RowFromRole(s, M_ROW_PICKUPS, stats, max_stats);
        }
        M_RowFromRole(s, M_ROW_KILLS, stats, max_stats);
        if (g_Config.ui.stat_detail_mode == SDM_FULL) {
            M_RowFromRole(s, M_ROW_AMMO, stats, max_stats);
        } else {
            M_RowFromRole(s, M_ROW_AMMO_USED, stats, max_stats);
            M_RowFromRole(s, M_ROW_AMMO_HITS, stats, max_stats);
        }
        M_RowFromRole(s, M_ROW_MEDIPACKS_USED, stats, max_stats);
        M_RowFromRole(s, M_ROW_DISTANCE_TRAVELLED, stats, max_stats);
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
    const RESUME_INFO *const current_info =
        Savegame_GetCurrentInfo(current_level);
    const STATS_COMMON *const stats =
        (const STATS_COMMON *)&current_info->stats;
    const LEVEL_MAX_STATS *const max_stats = &current_info->max_stats;

    if (g_Config.ui.enable_stats_level_header) {
        M_RowFromRole(s, M_ROW_LEVEL_COUNTER, stats, max_stats);
    }

    if (g_TRVersion == 1) {
        M_CommonRows(s, stats, max_stats);
    } else {
        M_RowFromRole(s, M_ROW_TIMER, stats, max_stats);
        if (max_stats->max_secret_count != 0) {
            M_RowFromRole(s, M_ROW_ICON_SECRETS, stats, max_stats);
        }
        M_CommonRows(s, stats, max_stats);
    }
}

static void M_FinalStatsRows(const UI_STATS_DIALOG_STATE *const s)
{
    const GF_LEVEL_TYPE level_type =
        GF_GetLevel(GFLT_MAIN, s->args.level_num)->type;
    const FINAL_STATS final_stats = Stats_ComputeFinalStats(level_type);
    const STATS_COMMON *const stats = &final_stats.stats;
    const LEVEL_MAX_STATS *const max_stats = &final_stats.max_stats;

    if (g_TRVersion == 1) {
        M_CommonRows(s, stats, max_stats);
    } else {
        M_RowFromRole(s, M_ROW_TIMER, stats, max_stats);
        if (max_stats->max_secret_count != 0) {
            M_RowFromRole(s, M_ROW_NUM_SECRETS, stats, max_stats);
        }
        M_CommonRows(s, stats, max_stats);
    }
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
            .spacing = { .v = s->look->window_pad },
            .align = { .h = UI_STACK_H_ALIGN_SPAN },
        });
    }
    // ensure minimum dialog width
    UI_Spacer(s->look->min_width, 0.0f);
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

UI_STATS_DIALOG_STATE *UI_StatsDialog_Init(const UI_STATS_DIALOG_ARGS args)
{
    UI_STATS_DIALOG_STATE *const s = Memory_Alloc(sizeof(*s));
    const ASSAULT_STATS stats = Gym_GetAssaultStats();
    int32_t max_assault_times = 0;
    for (int i = 0; i < MAX_ASSAULT_TIMES; i++) {
        if (stats.entries[i].time != 0) {
            max_assault_times++;
        }
    }
    UI_Requester_Init(&s->assault_req, 7, max_assault_times, false);
    s->assault_req.reserve_space = true;
    s->args = args;
    s->look = &m_Looks[g_TRVersion - 1];

    s->has_floordata_secrets = false;
    const GF_LEVEL *const level = Game_GetCurrentLevel();
    const LEVEL_MAX_STATS *const max_stats = Stats_GetLevelMaxStats(level);
    for (int32_t i = 0; i < STATS_MAX_SECRETS; i++) {
        if (max_stats->secret_objects[i].taken
            && max_stats->secret_objects[i].assigned_object_id == NO_OBJECT) {
            s->has_floordata_secrets = true;
        }
    }

    return s;
}

void UI_StatsDialog_Free(UI_STATS_DIALOG_STATE *const s)
{
    UI_Requester_Free(&s->assault_req);
}

int32_t UI_StatsDialog_Control(UI_STATS_DIALOG_STATE *const s)
{
    return UI_Requester_Control(&s->assault_req);
}

void UI_StatsDialog(UI_STATS_DIALOG_STATE *const s)
{
    UI_BeginModal(0.5f, s->look->window_y);
    UI_BeginPad(s->look->window_margin, s->look->window_margin);

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
