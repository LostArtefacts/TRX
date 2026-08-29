#include <trx/game/gym.h>

#include <trx/config.h>
#include <trx/config/section.h>
#include <trx/core/subsystem.h>
#include <trx/core/utils.h>
#include <trx/debug.h>
#include <trx/game/const.h>
#include <trx/game/game.h>
#include <trx/game/game_flow.h>
#include <trx/game/items.h>
#include <trx/game/lara.h>
#include <trx/game/music.h>
#include <trx/game/objects.h>
#include <trx/game/pathing/lot.h>
#include <trx/game/savegame.h>
#include <trx/game/stats.h>
#include <trx/version.h>

#define M_NO_TIME (-1)
#define M_MAX_ASSAULT_TIME_FRAMES (60 * 60 * LOGIC_FPS - 3) // 59:59

#define M_PRIV_INITIAL                                                         \
    {                                                                          \
        .is_inventory_open_enabled = -1,                                       \
    }

typedef struct {
    int32_t is_inventory_open_enabled;
    int16_t completion_timer;
    GYM_TRACK_TYPE active_track_type;

    struct {
        bool timer_display;
        bool timer_active;
        int32_t penalty_display_timer;
        int32_t penalty_frames;
        int32_t target_penalty_frames;
        int32_t timer_auto_hide_timer;
        bool pad_touched_this_frame;
        bool pad_lock;
    } assault_course;

    struct {
        bool timer_display;
        bool timer_active;
        int32_t lap_time;
        int32_t lap_time_display_timer;
    } quad_course;
} M_PRIV;

static M_PRIV m_Priv = M_PRIV_INITIAL;

// The player's own records, not the run's: they outlive a level and are read
// back from the settings file at startup. Kept apart from M_PRIV, which is
// reset whenever a course starts.
static GYM_TRACK_STATS m_AssaultStats = {};
static GYM_TRACK_STATS m_RacetrackStats = {};

static void M_Shutdown(void)
{
    m_Priv = (M_PRIV)M_PRIV_INITIAL;
}

static int32_t M_CountAssaultTargets(void)
{
    int32_t remaining = 0;
    for (int16_t item_num = 0; item_num < Item_GetLevelCount(); item_num++) {
        ITEM *const item = Item_Get(item_num);
        if (item->object_id != O_ASSAULT_TARGET) {
            continue;
        }

        if (!item->is_destroyed && item->hit_points > 0) {
            remaining++;
        }
    }
    return remaining;
}

static void M_ResetAssaultTargets(void)
{
    M_PRIV *const p = &m_Priv;

    const OBJECT *const obj = Object_Get(O_ASSAULT_TARGET);
    if (!obj->loaded) {
        return;
    }

    for (int16_t item_num = 0; item_num < Item_GetLevelCount(); item_num++) {
        ITEM *const item = Item_Get(item_num);
        if (item->object_id != O_ASSAULT_TARGET) {
            continue;
        }

        if (item->is_destroyed) {
            // Rockets can kill targets via Creature_Die()+Item_Destroy(), which
            // removes them from room draw + item lists. Use Item_Initialise()
            // to re-link them back.

            // Clear the seed so that Item_Initialise doesn't mark the item
            // as active preemptively.
            item->init_flags = 0;

            Item_Initialise(item_num);
        } else if (!Item_IsInactive(item) && obj->initialise_func != nullptr) {
            obj->initialise_func(item_num);
        }
    }
}

static int32_t M_GetBestTime(void)
{
    const GYM_TRACK_STATS *const assault = &m_AssaultStats;
    return assault->total_attempts > 0 ? (int32_t)assault->entries[0].time
                                       : M_NO_TIME;
}

static bool M_StoreCourseTime(GYM_TRACK_STATS *const stats, const uint32_t time)
{
    int32_t insert_idx = -1;
    const int32_t limit = MAX_ASSAULT_TIMES;
    for (int32_t i = 0; i < limit; i++) {
        if (stats->entries[i].time == 0 || time < stats->entries[i].time) {
            insert_idx = i;
            break;
        }
    }
    if (insert_idx == -1) {
        return false;
    }

    for (int32_t i = limit - 1; i > insert_idx; i--) {
        stats->entries[i] = stats->entries[i - 1];
    }

    stats->total_attempts++;
    stats->entries[insert_idx].time = time;
    stats->entries[insert_idx].attempt_num = stats->total_attempts;
    // The records are this module's, so nothing else can see that they moved.
    Config_SectionChanged();
    Config_Update();
    return true;
}

static bool M_IsOnQuadBike(void)
{
    const ITEM *const vehicle = Lara_Vehicle_GetItem();
    return vehicle != nullptr && vehicle->object_id == O_QUAD_BIKE;
}

static void M_Assault_Finish(void)
{
    M_PRIV *const p = &m_Priv;
    if (!p->assault_course.timer_active) {
        return;
    }

    RESUME_INFO *const resume = SG_Resume_GetEntry(Game_GetCurrentLevel());

    uint32_t final_time = resume->stats.timer;
    if (g_TRVersion >= 3) {
        const int32_t targets_remaining = M_CountAssaultTargets();
        p->assault_course.penalty_display_timer = 10 * LOGIC_FPS;
        p->assault_course.target_penalty_frames =
            10 * LOGIC_FPS * targets_remaining;
        CLAMPG(
            p->assault_course.target_penalty_frames, M_MAX_ASSAULT_TIME_FRAMES);

        final_time += (uint32_t)p->assault_course.penalty_frames
            + (uint32_t)p->assault_course.target_penalty_frames;
        CLAMPG(final_time, M_MAX_ASSAULT_TIME_FRAMES);
        resume->stats.timer = final_time;
    }

    if (g_TRVersion >= 3) {
        if (final_time < (uint32_t)(180 * LOGIC_FPS)) {
            Music_Play(MX_TR3_GYM_HINT_FAST_TIME, MPM_ONCE);
        }
        p->assault_course.timer_auto_hide_timer = 15 * LOGIC_FPS;
    } else {
        const int32_t current_best_time = M_GetBestTime();
        if (current_best_time <= 0) {
            if (final_time < (uint32_t)(100 * LOGIC_FPS)) {
                // "Gosh! That was my best time yet!"
                Music_Play(MX_TR2_GYM_HINT_15, MPM_ONCE);
            } else {
                // "Congratulations! You did it! But perhaps I could've been
                // faster."
                Music_Play(MX_TR2_GYM_HINT_17, MPM_ONCE);
            }
        } else if (final_time < (uint32_t)current_best_time) {
            // "Gosh! That was my best time yet!"
            Music_Play(MX_TR2_GYM_HINT_15, MPM_ONCE);
        } else if (final_time < (uint32_t)current_best_time + 5 * LOGIC_FPS) {
            // "Almost. Perhaps another try and I might beat it."
            Music_Play(MX_TR2_GYM_HINT_16, MPM_ONCE);
        } else {
            // "Great. But nowhere near my best time."
            Music_Play(MX_TR2_GYM_HINT_14, MPM_ONCE);
        }
    }

    p->assault_course.timer_active = false;

    M_StoreCourseTime(&m_AssaultStats, final_time);
}

static void M_Racetrack_Finish(void)
{
    if (!M_IsOnQuadBike()) {
        return;
    }

    M_PRIV *const p = &m_Priv;
    if (!p->quad_course.timer_active) {
        return;
    }

    RESUME_INFO *const resume = SG_Resume_GetEntry(Game_GetCurrentLevel());
    uint32_t final_time = resume->stats.timer;
    CLAMPG(final_time, M_MAX_ASSAULT_TIME_FRAMES);
    resume->stats.timer = final_time;

    p->quad_course.lap_time = (int32_t)final_time;
    p->quad_course.lap_time_display_timer = 10 * LOGIC_FPS;
    M_StoreCourseTime(&m_RacetrackStats, final_time);
    p->quad_course.timer_active = false;
}

// The records as the settings file carries them. The config module owns the
// file; what a record is made of is this module's own business.

static void M_NormalizeStats(GYM_TRACK_STATS *const stats)
{
    GYM_TRACK_ENTRY sorted_entries[MAX_ASSAULT_TIMES] = {};
    int32_t count = 0;
    uint32_t max_attempt_num = 0;

    for (int32_t i = 0; i < MAX_ASSAULT_TIMES; i++) {
        const GYM_TRACK_ENTRY entry = stats->entries[i];
        if (entry.time == 0) {
            continue;
        }

        if (entry.attempt_num > max_attempt_num) {
            max_attempt_num = entry.attempt_num;
        }

        int32_t insert_idx = count;
        while (insert_idx > 0
               && sorted_entries[insert_idx - 1].time > entry.time) {
            sorted_entries[insert_idx] = sorted_entries[insert_idx - 1];
            insert_idx--;
        }
        sorted_entries[insert_idx] = entry;
        count++;
    }

    for (int32_t i = 0; i < MAX_ASSAULT_TIMES; i++) {
        stats->entries[i] = sorted_entries[i];
    }

    if (stats->total_attempts < max_attempt_num) {
        stats->total_attempts = max_attempt_num;
    }
}

static void M_LoadStats(
    const JSON_OBJECT *const stats_obj, GYM_TRACK_STATS *const stats)
{
    if (stats_obj == nullptr) {
        return;
    }
    const JSON_ARRAY *const entries_arr =
        JSON_ObjectGetArray(stats_obj, "entries");
    if (entries_arr != nullptr) {
        for (size_t i = 0; i < entries_arr->length && i < MAX_ASSAULT_TIMES;
             i++) {
            const JSON_OBJECT *const entry_obj =
                JSON_ArrayGetObject(entries_arr, i);
            if (entry_obj != nullptr) {
                stats->entries[i].time = JSON_ObjectGetInt(
                    entry_obj, "time", stats->entries[i].time);
                stats->entries[i].attempt_num = JSON_ObjectGetInt(
                    entry_obj, "attempt_num", stats->entries[i].attempt_num);
            }
        }
    }
    stats->total_attempts =
        JSON_ObjectGetInt(stats_obj, "total_attempts", stats->total_attempts);
    M_NormalizeStats(stats);
}

static void M_SaveStats(
    JSON_OBJECT *const stats_obj, const GYM_TRACK_STATS *const stats)
{
    JSON_ARRAY *const entries_arr = JSON_ArrayNew();
    for (int32_t i = 0; i < MAX_ASSAULT_TIMES; i++) {
        if (stats->entries[i].time == 0) {
            break;
        }
        JSON_OBJECT *const entry_obj = JSON_ObjectNew();
        JSON_ObjectAppendInt(entry_obj, "time", stats->entries[i].time);
        JSON_ObjectAppendInt(
            entry_obj, "attempt_num", stats->entries[i].attempt_num);
        JSON_ArrayAppendObject(entries_arr, entry_obj);
    }
    JSON_ObjectAppendArray(stats_obj, "entries", entries_arr);
    JSON_ObjectAppendInt(stats_obj, "total_attempts", stats->total_attempts);
}

static void M_LoadAssaultSection(const JSON_OBJECT *const obj)
{
    M_LoadStats(obj, &m_AssaultStats);
}

static void M_SaveAssaultSection(JSON_OBJECT *const obj)
{
    // A game with no assault course has nothing to say here, and saying it
    // would drop the records the game that does have one left behind.
    if (Gym_TrackManager_HasStats(GYM_TRACK_ASSAULT)) {
        M_SaveStats(obj, &m_AssaultStats);
    }
}

static void M_LoadRacetrackSection(const JSON_OBJECT *const obj)
{
    M_LoadStats(obj, &m_RacetrackStats);
}

static void M_SaveRacetrackSection(JSON_OBJECT *const obj)
{
    if (Gym_TrackManager_HasStats(GYM_TRACK_QUAD)) {
        M_SaveStats(obj, &m_RacetrackStats);
    }
}

void Gym_SetInventoryOpenEnabled(const bool enabled)
{
    M_PRIV *const p = &m_Priv;
    p->is_inventory_open_enabled = enabled;
}

bool Gym_IsInventoryOpenEnabled(void)
{
    const M_PRIV *const p = &m_Priv;
    // Until the game says otherwise, the answer follows the engine version,
    // which changes under us when the player switches mods.
    if (p->is_inventory_open_enabled == -1) {
        return g_TRVersion >= 2;
    }
    return p->is_inventory_open_enabled;
}

void Gym_Control(void)
{
    if (g_TRVersion < 3) {
        return;
    }

    M_PRIV *const p = &m_Priv;

    if (p->assault_course.pad_lock
        && !p->assault_course.pad_touched_this_frame) {
        p->assault_course.pad_lock = false;
    }
    p->assault_course.pad_touched_this_frame = false;

    if (p->assault_course.penalty_display_timer > 0) {
        p->assault_course.penalty_display_timer--;
    }
    if (!p->assault_course.timer_active && p->assault_course.timer_display
        && p->assault_course.timer_auto_hide_timer > 0) {
        p->assault_course.timer_auto_hide_timer--;
        if (p->assault_course.timer_auto_hide_timer == 0) {
            p->assault_course.timer_display = false;
            p->active_track_type = GYM_TRACK_NONE;
        }
    }

    if (p->quad_course.lap_time_display_timer > 0) {
        p->quad_course.lap_time_display_timer--;
    }
}

GYM_TRACK_TYPE Gym_TrackManager_GetActiveTrackType(void)
{
    M_PRIV *const p = &m_Priv;
    return p->active_track_type;
}

bool Gym_TrackManager_HasStats(const GYM_TRACK_TYPE track)
{
    switch (track) {
    case GYM_TRACK_ASSAULT:
        return g_TRVersion >= 2;
    case GYM_TRACK_QUAD:
        return g_TRVersion >= 3;
    default:
        return false;
    }
}

GYM_TRACK_STATS *Gym_TrackManager_GetMutableStats(const GYM_TRACK_TYPE track)
{
    switch (track) {
    case GYM_TRACK_ASSAULT:
        return &m_AssaultStats;
    case GYM_TRACK_QUAD:
        return &m_RacetrackStats;
    default:
        return nullptr;
    }
}

const GYM_TRACK_STATS *Gym_TrackManager_GetStats(const GYM_TRACK_TYPE track)
{
    return Gym_TrackManager_GetMutableStats(track);
}

void Gym_TrackManager_ClearStats(const GYM_TRACK_TYPE track)
{
    GYM_TRACK_STATS *const stats = Gym_TrackManager_GetMutableStats(track);
    if (stats == nullptr) {
        return;
    }
    *stats = (GYM_TRACK_STATS) {};
    Config_SectionChanged();
    Config_Update();
}

bool Gym_TrackManager_IsTimerDisplay(const GYM_TRACK_TYPE track)
{
    M_PRIV *const p = &m_Priv;
    switch (track) {
    case GYM_TRACK_ASSAULT:
        return p->assault_course.timer_display;
    case GYM_TRACK_QUAD:
        return p->quad_course.timer_display;
    default:
        return false;
    }
}

bool Gym_TrackManager_IsTimerActive(const GYM_TRACK_TYPE track)
{
    M_PRIV *const p = &m_Priv;
    switch (track) {
    case GYM_TRACK_ASSAULT:
        return p->assault_course.timer_active;
    case GYM_TRACK_QUAD:
        return p->quad_course.timer_active;
    default:
        return false;
    }
}

void Gym_TrackManager_Reset(const GYM_TRACK_TYPE track)
{
    M_PRIV *const p = &m_Priv;
    p->active_track_type = GYM_TRACK_NONE;

    switch (track) {
    case GYM_TRACK_ASSAULT:
        p->assault_course.timer_active = false;
        p->assault_course.timer_display = false;
        p->assault_course.penalty_frames = 0;
        p->assault_course.target_penalty_frames = 0;
        p->assault_course.penalty_display_timer = 0;
        p->assault_course.timer_auto_hide_timer = 0;
        p->assault_course.pad_touched_this_frame = false;
        p->assault_course.pad_lock = false;
        break;

    case GYM_TRACK_QUAD:
        p->quad_course.timer_active = false;
        p->quad_course.timer_display = false;
        p->quad_course.lap_time = 0;
        p->quad_course.lap_time_display_timer = 0;
        break;

    default:
        break;
    }
}

void Gym_TrackManager_Start(const GYM_TRACK_TYPE track)
{
    if (track == GYM_TRACK_QUAD && !M_IsOnQuadBike()) {
        return;
    }

    M_PRIV *const p = &m_Priv;
    p->active_track_type = track;

    RESUME_INFO *const resume = SG_Resume_GetEntry(Game_GetCurrentLevel());
    resume->stats.timer = 0;

    switch (track) {
    case GYM_TRACK_ASSAULT: {
        p->quad_course.timer_active = false;
        p->quad_course.timer_display = false;
        p->quad_course.lap_time_display_timer = 0;

        p->assault_course.timer_active = true;
        p->assault_course.timer_display = true;
        p->assault_course.penalty_frames = 0;
        p->assault_course.target_penalty_frames = 0;
        p->assault_course.penalty_display_timer = 0;
        p->assault_course.timer_auto_hide_timer = 0;
        p->assault_course.pad_touched_this_frame = false;
        p->assault_course.pad_lock = false;
        M_ResetAssaultTargets();
        break;
    }

    case GYM_TRACK_QUAD: {
        p->assault_course.timer_active = false;
        p->assault_course.timer_display = false;
        p->assault_course.penalty_frames = 0;
        p->assault_course.target_penalty_frames = 0;
        p->assault_course.penalty_display_timer = 0;
        p->assault_course.timer_auto_hide_timer = 0;
        p->assault_course.pad_touched_this_frame = false;
        p->assault_course.pad_lock = false;

        p->quad_course.timer_active = true;
        p->quad_course.timer_display = true;
        break;
    }

    default:
        break;
    }
}

void Gym_TrackManager_Stop(const GYM_TRACK_TYPE track)
{
    M_PRIV *const p = &m_Priv;
    p->active_track_type = GYM_TRACK_NONE;

    switch (track) {
    case GYM_TRACK_ASSAULT:
        p->assault_course.timer_active = false;
        p->assault_course.timer_display = true;
        p->assault_course.timer_auto_hide_timer = 0;
        break;

    case GYM_TRACK_QUAD:
        if (!M_IsOnQuadBike()) {
            return;
        }
        p->quad_course.timer_active = false;
        p->quad_course.timer_display = true;
        break;

    default:
        break;
    }
}

void Gym_TrackManager_Finish(const GYM_TRACK_TYPE track)
{
    M_PRIV *const p = &m_Priv;

    switch (track) {
    case GYM_TRACK_ASSAULT:
        M_Assault_Finish();
        break;

    case GYM_TRACK_QUAD:
        M_Racetrack_Finish();
        break;

    default:
        break;
    }
}

void Gym_TrackManager_AddPenaltySeconds(
    const GYM_TRACK_TYPE track, const int32_t seconds)
{
    ASSERT(track == GYM_TRACK_ASSAULT);
    M_PRIV *const p = &m_Priv;
    if (seconds <= 0 || !p->assault_course.timer_active) {
        return;
    }

    p->assault_course.penalty_display_timer = 4 * LOGIC_FPS;
    p->assault_course.penalty_frames += seconds * LOGIC_FPS;
    CLAMPG(p->assault_course.penalty_frames, M_MAX_ASSAULT_TIME_FRAMES);
}

int32_t Gym_TrackManager_GetPenaltyDisplayTimer(const GYM_TRACK_TYPE track)
{
    if (track != GYM_TRACK_ASSAULT) {
        return 0;
    }
    M_PRIV *const p = &m_Priv;
    return p->assault_course.penalty_display_timer;
}

int32_t Gym_TrackManager_GetPenaltyFrames(const GYM_TRACK_TYPE track)
{
    if (track != GYM_TRACK_ASSAULT) {
        return 0;
    }
    M_PRIV *const p = &m_Priv;
    return p->assault_course.penalty_frames;
}

int32_t Gym_TrackManager_GetTargetPenaltyFrames(const GYM_TRACK_TYPE track)
{
    if (track != GYM_TRACK_ASSAULT) {
        return 0;
    }
    M_PRIV *const p = &m_Priv;
    return p->assault_course.target_penalty_frames;
}

bool Gym_TrackManager_OnPadContact(
    const GYM_TRACK_TYPE track, const bool on_ground)
{
    if (g_TRVersion < 3) {
        return true;
    }
    if (track != GYM_TRACK_ASSAULT) {
        return true;
    }
    if (!Game_IsInGym()) {
        return true;
    }

    M_PRIV *const p = &m_Priv;
    p->assault_course.pad_touched_this_frame = true;
    if (!on_ground) {
        return true;
    }
    if (p->assault_course.pad_lock) {
        return false;
    }

    p->assault_course.pad_lock = true;
    return true;
}

int32_t Gym_TrackManager_GetLapTimeDisplayTimer(const GYM_TRACK_TYPE track)
{
    if (track != GYM_TRACK_QUAD) {
        return 0;
    }
    M_PRIV *const p = &m_Priv;
    return p->quad_course.lap_time_display_timer;
}

int32_t Gym_TrackManager_GetLapTime(const GYM_TRACK_TYPE track)
{
    if (track != GYM_TRACK_QUAD) {
        return 0;
    }
    M_PRIV *const p = &m_Priv;
    return p->quad_course.lap_time;
}

bool Gym_CanPlayMusicTrack(MUSIC_SLOT *const track_id)
{
    const MUSIC_TRACK_STATE *const track_state = Music_GetTrackState(*track_id);
    const ITEM *const lara = Lara_GetItem();
    switch (Music_SlotToID(*track_id)) {
    case MX_TR1_GYM_HINT_03:
        if (track_state->is_one_shot
            && lara->current_anim_state == LS(LS_JUMP_UP)) {
            *track_id = Music_IDToSlot(MX_TR1_GYM_HINT_04);
        }
        break;

    case MX_TR1_GYM_HINT_12:
        if (lara->current_anim_state != LS(LS_HANG)) {
            return false;
        }
        break;

    case MX_TR1_GYM_HINT_16:
        if (lara->current_anim_state != LS(LS_HANG)) {
            return false;
        }
        break;

    case MX_TR1_GYM_HINT_17:
        if (track_state->is_one_shot
            && lara->current_anim_state == LS(LS_HANG)) {
            *track_id = Music_IDToSlot(MX_TR1_GYM_HINT_18);
        }
        break;

    case MX_TR1_GYM_HINT_24:
        if (lara->current_anim_state != LS(LS_SURF_TREAD)) {
            return false;
        }
        break;

    case MX_TR1_GYM_HINT_25:
        if (track_state->is_one_shot) {
            M_PRIV *const p = &m_Priv;
            p->completion_timer++;
            if (p->completion_timer == LOGIC_FPS * 4) {
                Game_SetIsLevelComplete(true);
                p->completion_timer = 0;
            }
        } else if (lara->current_anim_state != LS(LS_WATER_OUT)) {
            return false;
        }
        break;

    default:
        return true;
    }
    return true;
}

REGISTER_CONFIG_SECTION(
        .key = "assault_stats", .load = M_LoadAssaultSection,
        .save = M_SaveAssaultSection)

REGISTER_CONFIG_SECTION(
        .key = "racetrack_stats", .load = M_LoadRacetrackSection,
        .save = M_SaveRacetrackSection)

REGISTER_SUBSYSTEM(.shutdown = M_Shutdown)
