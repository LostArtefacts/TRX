#include <trx/config.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/debug.h>
#include <trx/game/const.h>
#include <trx/game/creature.h>
#include <trx/game/game.h>
#include <trx/game/game_flow.h>
#include <trx/game/gym.h>
#include <trx/game/objects.h>
#include <trx/game/objects/families.h>
#include <trx/game/savegame.h>
#include <trx/game/stats.h>
#include <trx/game/ui.h>
#include <trx/game/ui/scaler.h>
#include <trx/version.h>

#include <stdio.h>
#include <string.h>

#define M_MAX_ASSAULT_COURSE_ROWS 7
#define M_ASSAULT_COURSE_CHROME 80.0f
#define M_BARE_ROW_SPACING 11.0f
#define M_MIN_MARGIN_ROWS 0.5f
#define M_MIN_ROW_SPACING_ROWS (1.0f / 6.0f)

typedef enum {
    M_ROW_GENERIC,
    M_ROW_LEVEL_COUNTER,
    M_ROW_TIMER,
    M_ROW_AUTO_SECRETS,
    M_ROW_ICON_SECRETS,
    M_ROW_NUM_SECRETS,
    M_ROW_CRYSTALS,
    M_ROW_PICKUPS,
    M_ROW_DEATHS,
    M_ROW_KILLS,
    M_ROW_AMMO,
    M_ROW_AMMO_USED,
    M_ROW_AMMO_HITS,
    M_ROW_MEDIPACKS_USED,
    M_ROW_DISTANCE_TRAVELLED,
    M_ROW_ASSAULT_COURSE_TITLE,
    M_ROW_ASSAULT_COURSE_ROW,
    M_ROW_ASSAULT_NO_TIMES_SET,
    M_ROW_RACETRACK_TITLE,
    M_ROW_RACETRACK_ROW,
    M_ROW_SPACER,
} M_ROW_ROLE;

typedef struct {
    float window_margin;
    float window_y;
    float title_spacing;
    float min_width;
    float row_spacing;
    bool use_full_hours;
} M_LOOK;

typedef struct UI_STATS_DIALOG_STATE {
    UI_STATS_DIALOG_ARGS args;
    UI_SCROLLABLE scrollable;

    union {
        struct {
            const STATS_COMMON *stats;
            const LEVEL_MAX_STATS *max_stats;
            FINAL_STATS final_stats;
            LEVEL_MAX_STATS adjusted_max_stats;
        };
        const GYM_TRACK_STATS *assault_stats[GYM_TRACK_NUMBER_OF];
    };

    const M_LOOK *look;
    float window_margin;
    float bare_row_spacing;
    float fit_scale;
    bool is_squeezed;
    UI_PROGRESS_BUTTON_STATE *reset_button;
    bool has_floordata_secrets;
    bool has_visible_rows;
} UI_STATS_DIALOG_STATE;

static const M_LOOK m_Looks[TR_VERSION_COUNT] = {
    [0] = {
        .window_margin = 0.0f,
        .window_y = 0.5f,
        .title_spacing = 4.0f,
        .min_width = 0.0f,
        .row_spacing = 30.0f,
        .use_full_hours = false,
    },
    [1] = {
        .window_margin = 40.0f,
        .window_y = 1.0f,
        .title_spacing = 3.0f,
        .min_width = 290.0f,
        .row_spacing = 25.0f,
        .use_full_hours = true,
    },
    [2] = {
        .window_margin = 40.0f,
        .window_y = 1.0f,
        .title_spacing = 3.0f,
        .min_width = 290.0f,
        .row_spacing = 25.0f,
        .use_full_hours = true,
    },
    [3] = {
        .window_margin = 40.0f,
        .window_y = 1.0f,
        .title_spacing = 3.0f,
        .min_width = 290.0f,
        .row_spacing = 25.0f,
        .use_full_hours = true,
    },
};

static const char *M_FormatRecordTime(const int32_t total_frames)
{
    const int32_t total_seconds = total_frames / LOGIC_FPS;
    const int32_t minutes = (total_seconds / 60) % 60;
    const int32_t seconds = total_seconds % 60;
    const int32_t centiseconds = total_frames % LOGIC_FPS / (LOGIC_FPS / 10);
    return String_FormatStatic(
        "%02d:%02d.%-2d", minutes, seconds, centiseconds);
}

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

static const char *M_FormatDistance(uint32_t distance)
{
    distance /= 445;
    if (distance < 1000) {
        return String_FormatStatic("%dm", distance);
    } else {
        return String_FormatStatic(
            "%d.%02dkm", distance / 1000, (distance % 1000) / 10);
    }
}

static void M_AdjustMaxKills(
    UI_STATS_DIALOG_STATE *const s, const bool include_allies)
{
    if (s->max_stats == nullptr) {
        return;
    }
    s->adjusted_max_stats = *s->max_stats;
    s->adjusted_max_stats.maxes[STATS_CAT_KILLS] =
        s->adjusted_max_stats.max_kill_non_ally_count;
    if (include_allies) {
        s->adjusted_max_stats.maxes[STATS_CAT_KILLS] +=
            s->adjusted_max_stats.max_kill_ally_count;
    }
    s->max_stats = &s->adjusted_max_stats;
}

static bool M_HasHurtAlliesSoFar(const int32_t level_num)
{
    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
    if (level_table == nullptr || level_num <= 0) {
        return false;
    }
    for (int32_t i = 0; i <= level_num && i < level_table->count; i++) {
        const GF_LEVEL *const level = &level_table->levels[i];
        const RESUME_INFO *const resume = SG_Resume_GetEntry(level);
        if (resume != nullptr && resume->flags.available
            && resume->hurt_allies) {
            return true;
        }
    }
    return false;
}

static bool M_HasHurtAlliesEver(const bool include_bonus_levels)
{
    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
    if (level_table == nullptr) {
        return false;
    }
    for (int32_t i = 0; i < level_table->count; i++) {
        const GF_LEVEL *const level = &level_table->levels[i];
        if (!(level->type == GFL_NORMAL
              || (level->type == GFL_BONUS && include_bonus_levels))) {
            continue;
        }
        const RESUME_INFO *const resume = SG_Resume_GetEntry(level);
        if (resume != nullptr && resume->hurt_allies) {
            return true;
        }
    }
    return false;
}

static void M_FormatIconSecrets(char *const out, const GF_LEVEL *const level)
{
    char *ptr = out;
    int32_t num_secrets = 0;
    for (int32_t i = 0; i < STATS_MAX_SECRETS; i++) {
        if (!Stats_IsSecretValid(level, i)) {
            continue;
        }

        const bool has_secret = Stats_HasSecret(level, i);
        if (!has_secret && out == ptr) {
            // Do not reserve space pointlessly.
            // Good: [secret][ ][ ]
            // Bad:  [ ][ ][secret] – should be just [secret]
            continue;
        }
        const OBJECT_ID obj_id = Stats_GetSecretObject(level, i);
        if (obj_id != NO_OBJECT) {
            const int32_t secret_num =
                ObjectFamily_GetIndex(obj_id, OBJ_FAMILY_SECRET) + 1;
            ptr += sprintf(
                ptr, has_secret ? "\\{secret %d}" : "\\{i}\\{secret %d}\\{/i}",
                secret_num);
        }
        if (has_secret) {
            num_secrets++;
        }
    }
    *ptr++ = '\0';

    if (num_secrets == 0) {
        strcpy(out, GS("general/stats/none"));
    }
}

static void M_RowCentered(
    const UI_STATS_DIALOG_STATE *const s, const char *const text)
{
    UI_BeginStackEx((UI_STACK_SETTINGS) {
        .orientation = UI_STACK_VERTICAL,
        .align = { .h = UI_STACK_H_ALIGN_CENTER },
    });
    if (s->args.style == UI_STATS_DIALOG_STYLE_BARE) {
        char *text_upper = String_ToUpper(text);
        UI_Label(text_upper);
        Memory_FreePointer(&text_upper);
    } else {
        UI_Label(text);
    }
    UI_EndStack();
}

static void M_Row(
    const UI_STATS_DIALOG_STATE *const s, const char *const key,
    const char *const value)
{
    if (s->args.style == UI_STATS_DIALOG_STYLE_BARE) {
        char *key_upper = String_ToUpper(key);
        UI_BeginStack(UI_STACK_HORIZONTAL);
        UI_Label(key_upper);
        UI_Label(" ");
        UI_Label(value);
        UI_EndStack();
        Memory_FreePointer(&key_upper);
    } else {
        UI_BeginStackEx((UI_STACK_SETTINGS) {
            .orientation = UI_STACK_HORIZONTAL,
            .spacing = { .h = s->look->row_spacing },
            .align = { .h = UI_STACK_H_ALIGN_DISTRIBUTE },
        });
        UI_SetNodeName(UI_NODE_NAME_DIALOG_ROW);
        UI_Label(key);
        UI_Label(value);
        UI_EndStack();
    }
}

static void M_RowFromRole(
    const UI_STATS_DIALOG_STATE *const s, const M_ROW_ROLE role,
    const int32_t param)
{
    const char *const num_fmt = g_Config.ui.stats.show_totals
        ? GS("general/stats/detail_fmt")
        : GS("general/stats/basic_fmt");

    switch (role) {
    case M_ROW_LEVEL_COUNTER:
        M_Row(
            s, GS("general/stats/level"),
            String_FormatStatic(
                GS("general/stats/detail_fmt"), s->args.display_level_num,
                GF_GetLevelCount(GFLT_MAIN)));
        break;

    case M_ROW_TIMER:
        M_Row(
            s, GS("general/stats/time_taken"),
            M_FormatTime(s, s->stats->timer));
        break;

    case M_ROW_AUTO_SECRETS:
        if (s->args.mode == UI_STATS_DIALOG_MODE_FINAL
            || s->has_floordata_secrets) {
            M_RowFromRole(s, M_ROW_NUM_SECRETS, 0);
        } else {
            M_RowFromRole(s, M_ROW_ICON_SECRETS, 0);
        }
        break;

    case M_ROW_ICON_SECRETS: {
        char buf[256];
        M_FormatIconSecrets(buf, GF_GetLevel(GFLT_MAIN, s->args.level_num));
        M_Row(s, GS("general/stats/secrets"), buf);
        break;
    }

    case M_ROW_NUM_SECRETS:
        M_Row(
            s, GS("general/stats/secrets"),
            String_FormatStatic(
                GS("general/stats/detail_fmt"),
                s->stats->counts[STATS_CAT_SECRETS],
                s->max_stats->maxes[STATS_CAT_SECRETS]));
        break;

    case M_ROW_CRYSTALS:
        M_Row(
            s, GS("general/stats/crystals"),
            String_FormatStatic(
                num_fmt, s->stats->counts[STATS_CAT_CRYSTALS],
                s->max_stats->maxes[STATS_CAT_CRYSTALS]));
        break;

    case M_ROW_PICKUPS:
        M_Row(
            s, GS("general/stats/pickups"),
            String_FormatStatic(
                num_fmt, s->stats->counts[STATS_CAT_PICKUPS],
                s->max_stats->maxes[STATS_CAT_PICKUPS]));
        break;

    case M_ROW_KILLS:
        M_Row(
            s, GS("general/stats/kills"),
            String_FormatStatic(
                num_fmt, s->stats->counts[STATS_CAT_KILLS],
                s->max_stats->maxes[STATS_CAT_KILLS]));
        break;

    case M_ROW_DEATHS:
        M_Row(
            s, GS("general/stats/deaths"),
            String_FormatStatic(
                GS("general/stats/basic_fmt"), s->stats->death_count));
        break;

    case M_ROW_AMMO:
        M_Row(
            s, GS("general/stats/ammo"),
            String_FormatStatic(
                GS("general/misc/pagination_nav"), s->stats->ammo_hits,
                s->stats->ammo_used));
        break;

    case M_ROW_AMMO_USED:
        M_Row(
            s, GS("general/stats/ammo_used"),
            String_FormatStatic("%d", s->stats->ammo_used));
        break;

    case M_ROW_AMMO_HITS:
        M_Row(
            s, GS("general/stats/ammo_hits"),
            String_FormatStatic("%d", s->stats->ammo_hits));
        break;

    case M_ROW_MEDIPACKS_USED:
        M_Row(
            s, GS("general/stats/medipacks_used"),
            String_FormatStatic("%.1f", s->stats->medipacks_used));
        break;

    case M_ROW_DISTANCE_TRAVELLED:
        M_Row(
            s, GS("general/stats/distance_travelled"),
            M_FormatDistance(s->stats->distance_travelled));
        break;

    case M_ROW_ASSAULT_COURSE_TITLE:
        M_RowCentered(s, GS("general/stats/gym_assault_course"));
        break;

    case M_ROW_ASSAULT_COURSE_ROW:
    case M_ROW_RACETRACK_ROW: {
        const GYM_TRACK_TYPE track_type = role == M_ROW_ASSAULT_COURSE_ROW
            ? GYM_TRACK_ASSAULT
            : GYM_TRACK_QUAD;
        const GYM_TRACK_ENTRY *const entry =
            &s->assault_stats[track_type]->entries[param];
        const char *const attempt_str = String_FormatStatic(
            "%2d: %s %d", param + 1, GS("general/stats/assault_finish"),
            entry->attempt_num);
        const char *const time_str = String_FormatStatic(
            param == 0 ? GS("general/stats/assault_best_time_fmt")
                       : GS("general/stats/assault_other_times_fmt"),
            M_FormatRecordTime(entry->time));
        if (g_TRVersion == 3) {
            M_RowCentered(s, time_str);
        } else {
            M_Row(s, attempt_str, time_str);
        }
        break;
    }

    case M_ROW_ASSAULT_NO_TIMES_SET:
        M_RowCentered(s, GS("general/stats/assault_no_times_set"));
        break;

    case M_ROW_RACETRACK_TITLE:
        M_RowCentered(s, GS("general/stats/gym_racetrack_course"));
        break;

    case M_ROW_SPACER:
        M_RowCentered(s, " ");
        break;

    default:
        break;
    }
}

static void M_EmitRow(
    const UI_STATS_DIALOG_STATE *const s, const M_ROW_ROLE role,
    const int32_t param)
{
    M_RowFromRole(s, role, param);
}

static void M_EmitDummyRow(
    const UI_STATS_DIALOG_STATE *const s, const M_ROW_ROLE role,
    const int32_t param)
{
}

// Pickup crystals exist only to be counted, so their row is not optional.
static bool M_ShowCrystals(void)
{
    return g_Config.ui.stats.show_crystals
        || g_Config.gameplay.save_crystal_mode == SAVE_CRYSTAL_PICKUP;
}

static int32_t M_EmitConfiguredStatsRows(
    const UI_STATS_DIALOG_STATE *const s, const bool dry_run)
{
    int32_t rows = 0;
    void (*emit_row_func)(const UI_STATS_DIALOG_STATE *, M_ROW_ROLE, int32_t) =
        dry_run ? M_EmitDummyRow : M_EmitRow;

#define L_EMIT_ROW(...)                                                        \
    emit_row_func(__VA_ARGS__);                                                \
    rows++;

    if (s->args.style == UI_STATS_DIALOG_STYLE_BARE) {
        if (g_Config.ui.stats.show_kills) {
            L_EMIT_ROW(s, M_ROW_KILLS, 0);
        }
        if (g_Config.ui.stats.show_pickups) {
            L_EMIT_ROW(s, M_ROW_PICKUPS, 0);
        }
        if (M_ShowCrystals() && s->max_stats->maxes[STATS_CAT_CRYSTALS] != 0) {
            L_EMIT_ROW(s, M_ROW_CRYSTALS, 0);
        }
        if (g_Config.ui.stats.show_secrets
            && s->max_stats->maxes[STATS_CAT_SECRETS] != 0) {
            L_EMIT_ROW(s, M_ROW_AUTO_SECRETS, 0);
        }
        if (g_Config.ui.stats.show_time_taken) {
            L_EMIT_ROW(s, M_ROW_TIMER, 0);
        }
    } else {
        if (g_Config.ui.stats.show_time_taken) {
            L_EMIT_ROW(s, M_ROW_TIMER, 0);
        }
        if (g_Config.ui.stats.show_secrets
            && s->max_stats->maxes[STATS_CAT_SECRETS] != 0) {
            L_EMIT_ROW(s, M_ROW_AUTO_SECRETS, 0);
        }
        if (M_ShowCrystals() && s->max_stats->maxes[STATS_CAT_CRYSTALS] != 0) {
            L_EMIT_ROW(s, M_ROW_CRYSTALS, 0);
        }
        if (g_Config.ui.stats.show_pickups) {
            L_EMIT_ROW(s, M_ROW_PICKUPS, 0);
        }
        if (g_Config.ui.stats.show_kills) {
            L_EMIT_ROW(s, M_ROW_KILLS, 0);
        }
    }

    if (g_Config.ui.stats.show_ammo) {
        if (s->args.style == UI_STATS_DIALOG_STYLE_BARE) {
            L_EMIT_ROW(s, M_ROW_AMMO_USED, 0);
            L_EMIT_ROW(s, M_ROW_AMMO_HITS, 0);
        } else {
            L_EMIT_ROW(s, M_ROW_AMMO, 0);
        }
    }
    if (g_Config.ui.stats.show_medipacks_used) {
        L_EMIT_ROW(s, M_ROW_MEDIPACKS_USED, 0);
    }
    if (g_Config.ui.stats.show_distance_travelled) {
        L_EMIT_ROW(s, M_ROW_DISTANCE_TRAVELLED, 0);
    }
    if (g_Config.ui.stats.show_deaths && s->stats->death_count >= 0) {
        // Always use the sum of all levels for deaths.
        // Deaths get stored in the resume info for the level they happen on,
        // so if the player dies in Vilcabamba and reloads Caves, they should
        // still see an incremented death counter.
        L_EMIT_ROW(s, M_ROW_DEATHS, 0);
    }

#undef L_EMIT_ROW
    return rows;
}

static bool M_HasVisibleRows(const UI_STATS_DIALOG_STATE *const s)
{
    if (s->args.mode == UI_STATS_DIALOG_MODE_LEVEL
        && g_Config.ui.stats.show_level_header) {
        return true;
    }

    return M_EmitConfiguredStatsRows(s, true) > 0;
}

static void M_LevelStatsRows(const UI_STATS_DIALOG_STATE *const s)
{
    if (g_Config.ui.stats.show_level_header) {
        M_EmitRow(s, M_ROW_LEVEL_COUNTER, 0);
    }
    if (!s->has_visible_rows) {
        M_RowCentered(s, GS("general/osd/complete_level"));
        return;
    }
    M_EmitConfiguredStatsRows(s, false);
}

static void M_FinalStatsRows(const UI_STATS_DIALOG_STATE *const s)
{
    M_EmitConfiguredStatsRows(s, false);
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
            ? GS("general/stats/bonus_statistics")
            : GS("general/stats/final_statistics");
        return title;
    }
    case UI_STATS_DIALOG_MODE_ASSAULT_COURSE:
        return GS("general/stats/assault_title");
    }
    return nullptr;
}

static int32_t M_GetAssaultCourseMaxRows(void)
{
    const int32_t record_limit = g_TRVersion >= 3 ? 3 : MAX_ASSAULT_TIMES;
    const int32_t rows = Gym_TrackManager_HasStats(GYM_TRACK_QUAD)
        ? 3 + 2 * record_limit
        : record_limit;
    const float available =
        UI_GetSafeCanvasHeight() / UI_Scaler_GetBaseTextScale()
        - M_ASSAULT_COURSE_CHROME;
    int32_t result =
        MIN(MIN(rows, M_MAX_ASSAULT_COURSE_ROWS),
            (int32_t)(available / UI_TEXT_HEIGHT));
    CLAMP(result, 1, rows);
    return result;
}

static void M_AssaultCourseStatsRows(UI_STATS_DIALOG_STATE *const s)
{
    const int32_t record_limit = g_TRVersion >= 3 ? 3 : MAX_ASSAULT_TIMES;
    const bool has_race_track = Gym_TrackManager_HasStats(GYM_TRACK_QUAD);
    int32_t count = 0;

#define L_EMIT_ROW(...)                                                        \
    M_RowFromRole(__VA_ARGS__);                                                \
    count++;

    if (has_race_track) {
        L_EMIT_ROW(s, M_ROW_ASSAULT_COURSE_TITLE, 0);
    }

    if (s->assault_stats[GYM_TRACK_ASSAULT]->entries[0].time == 0) {
        L_EMIT_ROW(s, M_ROW_ASSAULT_NO_TIMES_SET, 0);
    } else {
        for (int32_t i = 0; i < record_limit; i++) {
            if (s->assault_stats[GYM_TRACK_ASSAULT]->entries[i].time == 0) {
                break;
            }
            L_EMIT_ROW(s, M_ROW_ASSAULT_COURSE_ROW, i);
        }
    }

    if (has_race_track) {
        L_EMIT_ROW(s, M_ROW_SPACER, 0);
        L_EMIT_ROW(s, M_ROW_RACETRACK_TITLE, 0);
        if (s->assault_stats[GYM_TRACK_QUAD]->entries[0].time == 0) {
            L_EMIT_ROW(s, M_ROW_ASSAULT_NO_TIMES_SET, 0);
        } else {
            for (int32_t i = 0; i < record_limit; i++) {
                if (s->assault_stats[GYM_TRACK_QUAD]->entries[i].time == 0) {
                    break;
                }
                L_EMIT_ROW(s, M_ROW_RACETRACK_ROW, i);
            }
        }
    }

#undef L_EMIT_ROW

    while (count < M_GetAssaultCourseMaxRows()) {
        M_RowFromRole(s, M_ROW_SPACER, 0);
        count++;
    }
}

static int32_t M_GetAssaultCourseRowCount(const UI_STATS_DIALOG_STATE *const s)
{
    const int32_t record_limit = g_TRVersion >= 3 ? 3 : MAX_ASSAULT_TIMES;
    const bool has_race_track = Gym_TrackManager_HasStats(GYM_TRACK_QUAD);
    int32_t count = 0;

    if (has_race_track) {
        count++;
    }

    if (s->assault_stats[GYM_TRACK_ASSAULT]->entries[0].time == 0) {
        count++;
    } else {
        for (int32_t i = 0; i < record_limit; i++) {
            if (s->assault_stats[GYM_TRACK_ASSAULT]->entries[i].time == 0) {
                break;
            }
            count++;
        }
    }

    if (has_race_track) {
        count += 2;
        if (s->assault_stats[GYM_TRACK_QUAD]->entries[0].time == 0) {
            count++;
        } else {
            for (int32_t i = 0; i < record_limit; i++) {
                if (s->assault_stats[GYM_TRACK_QUAD]->entries[i].time == 0) {
                    break;
                }
                count++;
            }
        }
    }

    return MAX(count, M_GetAssaultCourseMaxRows());
}

static bool M_HasAnyRecordedTime(const UI_STATS_DIALOG_STATE *const s)
{
    for (GYM_TRACK_TYPE track = 0; track < GYM_TRACK_NUMBER_OF; track++) {
        if (s->assault_stats[track]->entries[0].time != 0) {
            return true;
        }
    }
    return false;
}

static void M_ResetAssaultCourseStats(void *const user_data)
{
    UI_STATS_DIALOG_STATE *const s = user_data;
    for (GYM_TRACK_TYPE track = 0; track < GYM_TRACK_NUMBER_OF; track++) {
        Gym_TrackManager_ClearStats(track);
    }
    UI_Scrollable_SetMaxItems(&s->scrollable, M_GetAssaultCourseRowCount(s));
    UI_Scrollable_SelectFirstItem(&s->scrollable);
}

static UI_WINDOW_SETTINGS M_GetWindowSettings(
    const UI_STATS_DIALOG_STATE *const s)
{
    const bool is_assault_mode =
        s->args.mode == UI_STATS_DIALOG_MODE_ASSAULT_COURSE;
    return (UI_WINDOW_SETTINGS) {
        .title = M_GetDialogTitle(s),
        .scrollable = is_assault_mode ? &s->scrollable : nullptr,
        .title_spacing = s->look->title_spacing,
    };
}

static int32_t M_CountRows(const UI_STATS_DIALOG_STATE *const s)
{
    switch (s->args.mode) {
    case UI_STATS_DIALOG_MODE_LEVEL:
    case UI_STATS_DIALOG_MODE_FINAL: {
        int32_t rows = MAX(1, M_EmitConfiguredStatsRows(s, true));
        if (s->args.mode == UI_STATS_DIALOG_MODE_LEVEL
            && g_Config.ui.stats.show_level_header) {
            rows++;
        }
        return rows;
    }

    case UI_STATS_DIALOG_MODE_ASSAULT_COURSE:
        return s->scrollable.vis_items + 1;
    }
    return 0;
}

static float M_GetRowHeight(void)
{
    float height = 0.0f;
    UI_Label_Measure("0", nullptr, &height);
    return height / UI_Scaler_GetTextScale();
}

static float M_GetTextHeight(
    const UI_STATS_DIALOG_STATE *const s, const int32_t rows,
    const float row_height)
{
    float height = rows * row_height;
    if (s->args.style == UI_STATS_DIALOG_STYLE_BARE) {
        height += row_height;
    } else {
        const UI_WINDOW_SETTINGS window = M_GetWindowSettings(s);
        height += UI_Window_GetChromeHeight(&window);
    }
    return height;
}

static float M_GetContentHeight(
    const UI_STATS_DIALOG_STATE *const s, const int32_t rows,
    const float row_height)
{
    float height =
        M_GetTextHeight(s, rows, row_height) + 2.0f * s->window_margin;
    if (s->args.style == UI_STATS_DIALOG_STYLE_BARE) {
        height += (rows + 1) * s->bare_row_spacing;
    }
    return height;
}

static float M_GetAvailableHeight(void)
{
    return UI_GetSafeCanvasHeight() / UI_Scaler_GetTextScale();
}

static void M_RecomputeLayout(
    UI_STATS_DIALOG_STATE *const s, const int32_t rows, const float row_height)
{
    const bool is_bare = s->args.style == UI_STATS_DIALOG_STYLE_BARE;
    const float natural_spacing = is_bare ? M_BARE_ROW_SPACING : 0.0f;
    const float natural_margin = s->look->window_margin;
    const int32_t gaps = is_bare ? rows + 1 : 0;

    s->window_margin = natural_margin;
    s->bare_row_spacing = natural_spacing;
    s->fit_scale = 1.0f;

    const float available = M_GetAvailableHeight();
    const float text_height = M_GetTextHeight(s, rows, row_height);
    const float natural = 2.0f * natural_margin + gaps * natural_spacing;
    s->is_squeezed = text_height + natural > available;
    if (!s->is_squeezed) {
        return;
    }

    const float min_margin =
        MIN(row_height * M_MIN_MARGIN_ROWS, natural_margin);
    const float min_spacing =
        MIN(row_height * M_MIN_ROW_SPACING_ROWS, natural_spacing);

    float excess = text_height + natural - available;
    const float spare_spacing = gaps * (natural_spacing - min_spacing);
    s->bare_row_spacing =
        gaps > 0 ? natural_spacing - MIN(excess, spare_spacing) / gaps : 0.0f;

    excess -= spare_spacing;
    if (excess > 0.0f) {
        const float spare_margin = 2.0f * (natural_margin - min_margin);
        s->window_margin = natural_margin - MIN(excess, spare_margin) / 2.0f;
    }

    s->fit_scale =
        UI_GetFitScale(-1.0f, M_GetContentHeight(s, rows, row_height));
}

static void M_BeginDialog(const UI_STATS_DIALOG_STATE *const s)
{
    if (s->args.style == UI_STATS_DIALOG_STYLE_BARE) {
        UI_BeginStackEx((UI_STACK_SETTINGS) {
            .orientation = UI_STACK_VERTICAL,
            .spacing = { .v = s->bare_row_spacing },
            .align = { .h = UI_STACK_H_ALIGN_CENTER },
        });
        const char *const title = M_GetDialogTitle(s);
        if (title != nullptr) {
            UI_Label(title);
        }
    } else {
        UI_BeginWindow(M_GetWindowSettings(s));
    }
    // ensure minimum dialog width
    UI_Spacer(s->look->min_width, 0.0f);
}

static void M_EndDialog(const UI_STATS_DIALOG_STATE *const s)
{
    if (s->args.style == UI_STATS_DIALOG_STYLE_BARE) {
        UI_EndStack();
    } else {
        UI_EndWindow();
    }
}

UI_STATS_DIALOG_STATE *UI_StatsDialog_Init(const UI_STATS_DIALOG_ARGS args)
{
    UI_STATS_DIALOG_STATE *const s = Memory_Alloc(sizeof(*s));

    s->has_floordata_secrets = false;
    s->scrollable.vis_items = M_GetAssaultCourseMaxRows();
    s->args = args;
    s->look = &m_Looks[g_TRVersion - 1];

    switch (args.mode) {
    case UI_STATS_DIALOG_MODE_LEVEL:
        const GF_LEVEL *const current_level =
            GF_GetLevel(GFLT_MAIN, s->args.level_num);
        const RESUME_INFO *const current_info =
            SG_Resume_GetEntry(current_level);
        s->stats = (const STATS_COMMON *)&current_info->stats;
        s->max_stats = Stats_GetLevelMaxStats(current_level);
        const bool include_allies = M_HasHurtAlliesSoFar(s->args.level_num);
        M_AdjustMaxKills(s, include_allies);

        const GF_LEVEL *const level = Game_GetCurrentLevel();
        for (int32_t i = 0; i < STATS_MAX_SECRETS; i++) {
            if (s->max_stats->secret_objects[i].taken
                && s->max_stats->secret_objects[i].assigned_object_id
                    == NO_OBJECT) {
                s->has_floordata_secrets = true;
            }
        }
        s->has_visible_rows = M_HasVisibleRows(s);
        break;

    case UI_STATS_DIALOG_MODE_FINAL:
        const GF_LEVEL_TYPE level_type =
            GF_GetLevel(GFLT_MAIN, s->args.level_num)->type;
        const bool include_bonus_levels = level_type == GFL_BONUS;
        s->final_stats = Stats_ComputeFinalStats(include_bonus_levels);
        s->stats = &s->final_stats.stats;
        s->max_stats = &s->final_stats.max_stats;
        M_AdjustMaxKills(s, M_HasHurtAlliesEver(include_bonus_levels));
        s->has_visible_rows = M_HasVisibleRows(s);
        break;

    case UI_STATS_DIALOG_MODE_ASSAULT_COURSE:
        for (int32_t track_type = 0; track_type < GYM_TRACK_NUMBER_OF;
             track_type++) {
            s->assault_stats[track_type] =
                Gym_TrackManager_GetStats(track_type);
        }
        s->scrollable.max_items = M_GetAssaultCourseRowCount(s);
        s->has_visible_rows = true;
        s->reset_button = UI_ProgressButton_Init(
            g_Config.input.backend, INPUT_ROLE_UNBIND_KEY,
            GS_ID("general/stats/assault_reset_times"),
            M_ResetAssaultCourseStats, s);
        break;
    }

    return s;
}

void UI_StatsDialog_Free(UI_STATS_DIALOG_STATE *const s)
{
    if (s->reset_button != nullptr) {
        UI_ProgressButton_Free(s->reset_button);
    }
    Memory_Free(s);
}

bool UI_StatsDialog_HasVisibleRows(const UI_STATS_DIALOG_STATE *const s)
{
    return s->has_visible_rows;
}

int32_t UI_StatsDialog_Control(UI_STATS_DIALOG_STATE *const s)
{
    if (s->reset_button != nullptr && M_HasAnyRecordedTime(s)) {
        UI_ProgressButton_Control(s->reset_button);
    }
    return UI_ScrollableStack_Control(&s->scrollable, UI_STACK_VERTICAL);
}

void UI_StatsDialog(UI_STATS_DIALOG_STATE *const s)
{
    const int32_t rows = M_CountRows(s);
    const float row_height = M_GetRowHeight();
    M_RecomputeLayout(s, rows, row_height);
    UI_Scaler_PushTextScale(s->fit_scale);
    UI_BeginModal(0.5f, s->is_squeezed ? 0.5f : s->look->window_y);
    UI_BeginPad(s->window_margin, s->window_margin);

    M_BeginDialog(s);
    switch (s->args.mode) {
    case UI_STATS_DIALOG_MODE_LEVEL:
        M_LevelStatsRows(s);
        break;

    case UI_STATS_DIALOG_MODE_FINAL:
        M_FinalStatsRows(s);
        break;

    case UI_STATS_DIALOG_MODE_ASSAULT_COURSE:
        // Ensure minimum size even if there are no items
        UI_Spacer(290.0f, 0.0f);
        UI_BeginScrollableStack(
            &s->scrollable,
            (UI_SCROLLABLE_STACK_SETTINGS) {
                .orientation = UI_STACK_VERTICAL,
                .spacing = 3.0f,
            });
        M_AssaultCourseStatsRows(s);
        UI_EndScrollableStack();
        UI_BeginHide(!M_HasAnyRecordedTime(s));
        UI_BeginAnchor(0.5f, 0.5f);
        UI_ProgressButton(s->reset_button);
        UI_EndAnchor();
        UI_EndHide();
        break;
    }

    M_EndDialog(s);

    UI_EndPad();
    UI_EndModal();
    UI_Scaler_PopTextScale();
}
