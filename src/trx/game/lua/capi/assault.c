#include <trx/config.h>
#include <trx/config/types.h>
#include <trx/game/const.h>
#include <trx/game/game.h>
#include <trx/game/gym.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>
#include <trx/game/savegame/resume.h>

#include <lauxlib.h>
#include <stdint.h>

static bool M_StoreTime(GYM_TRACK_STATS *const stats, const float time)
{
    uint32_t logic_time = (uint32_t)(time * LOGIC_FPS);
    int32_t insert_idx = -1;

    for (int32_t i = 0; i < MAX_ASSAULT_TIMES; i++) {
        if (stats->entries[i].time == 0
            || logic_time < stats->entries[i].time) {
            insert_idx = i;
            break;
        }
    }
    if (insert_idx == -1) {
        return false;
    }

    for (int32_t i = MAX_ASSAULT_TIMES - 1; i > insert_idx; i--) {
        stats->entries[i] = stats->entries[i - 1];
    }

    stats->total_attempts++;
    stats->entries[insert_idx].time = logic_time;
    stats->entries[insert_idx].attempt_num = stats->total_attempts;
    Config_Update();
    return true;
}

static bool M_RemoveTimeAtIndex(GYM_TRACK_STATS *const stats, const int32_t idx)
{
    if (idx < 0 || idx >= MAX_ASSAULT_TIMES) {
        return false;
    }
    if (stats->entries[idx].time == 0) {
        return false;
    }

    for (int32_t i = idx; i < MAX_ASSAULT_TIMES - 1; i++) {
        stats->entries[i] = stats->entries[i + 1];
    }

    stats->entries[MAX_ASSAULT_TIMES - 1].time = 0;
    stats->entries[MAX_ASSAULT_TIMES - 1].attempt_num = 0;
    Config_Update();
    return true;
}

static GYM_TRACK_TYPE M_GetTrackAt(lua_State *const L, const int arg)
{
    const lua_Integer raw_track = luaL_optinteger(L, arg, GYM_TRACK_ASSAULT);

    switch (raw_track) {
    case GYM_TRACK_ASSAULT:
    case GYM_TRACK_QUAD:
        return (GYM_TRACK_TYPE)raw_track;
    default:
        luaL_argerror(L, arg, "unknown assault track");
        return GYM_TRACK_ASSAULT;
    }
}

static GYM_TRACK_TYPE M_GetTrack(lua_State *const L)
{
    return M_GetTrackAt(L, 1);
}

static const char *M_GetTrackName(const GYM_TRACK_TYPE track)
{
    switch (track) {
    case GYM_TRACK_ASSAULT:
        return "Assault course";
    case GYM_TRACK_QUAD:
        return "Quad Bike";
    default:
        return "Unknown";
    }
}

static void M_CheckTimerAvailable(
    lua_State *const L, const GYM_TRACK_TYPE track)
{
    if (!Game_IsInGym()) {
        luaL_error(
            L, "the %s timer is only available in gym levels",
            M_GetTrackName(track));
    }

    if (!Gym_TrackManager_HasStats(track)) {
        luaL_error(L, "the %s timer is unavailable", M_GetTrackName(track));
    }
}

// The record table the track keeps. No gym check, unlike the timers: the
// records live in the player's profile rather than in the level, and it is
// which game this is that decides whether a track has a table at all.
static GYM_TRACK_STATS *M_CheckStats(
    lua_State *const L, const GYM_TRACK_TYPE track)
{
    GYM_TRACK_STATS *const stats = Gym_TrackManager_GetMutableStats(track);
    if (stats == nullptr || !Gym_TrackManager_HasStats(track)) {
        luaL_error(L, "the %s records are unavailable", M_GetTrackName(track));
    }
    return stats;
}

// trxc.assault.start([track])
static int M_L_AssaultStart(lua_State *const L)
{
    const GYM_TRACK_TYPE track = M_GetTrack(L);
    M_CheckTimerAvailable(L, track);
    Gym_TrackManager_Start(track);
    return 0;
}

// trxc.assault.stop([track])
static int M_L_AssaultStop(lua_State *const L)
{
    const GYM_TRACK_TYPE track = M_GetTrack(L);
    M_CheckTimerAvailable(L, track);
    Gym_TrackManager_Stop(track);
    return 0;
}

// trxc.assault.reset([track])
static int M_L_AssaultReset(lua_State *const L)
{
    const GYM_TRACK_TYPE track = M_GetTrack(L);
    M_CheckTimerAvailable(L, track);
    Gym_TrackManager_Reset(track);
    return 0;
}

// trxc.assault.finish([track])
static int M_L_AssaultFinish(lua_State *const L)
{
    const GYM_TRACK_TYPE track = M_GetTrack(L);
    M_CheckTimerAvailable(L, track);
    Gym_TrackManager_Finish(track);
    return 0;
}

// trxc.assault.is_running([track]) -> bool
//
// No availability check: asking whether a timer runs is a fair question outside
// a gym level, and the answer there is false.
static int M_L_AssaultIsRunning(lua_State *const L)
{
    lua_pushboolean(L, Gym_TrackManager_IsTimerActive(M_GetTrack(L)));
    return 1;
}

// trxc.assault.is_visible([track]) -> bool
static int M_L_AssaultIsVisible(lua_State *const L)
{
    lua_pushboolean(L, Gym_TrackManager_IsTimerDisplay(M_GetTrack(L)));
    return 1;
}

// trxc.assault.get_time() -> frames
static int M_L_AssaultGetTime(lua_State *const L)
{
    const GF_LEVEL *const level = Game_GetCurrentLevel();
    const RESUME_INFO *const resume =
        level == nullptr ? nullptr : SG_Resume_GetEntry(level);
    lua_pushinteger(
        L, resume == nullptr ? 0 : (lua_Integer)resume->stats.timer);
    return 1;
}

// trxc.assault.get_best_time([track]) -> frames
static int M_L_AssaultGetBestTime(lua_State *const L)
{
    const GYM_TRACK_TYPE track = M_GetTrack(L);
    const GYM_TRACK_STATS *const stats = Gym_TrackManager_GetStats(track);
    const bool has_time = stats != nullptr && stats->total_attempts > 0;
    lua_pushinteger(L, has_time ? (lua_Integer)stats->entries[0].time : 0);
    return 1;
}

// trxc.assault.get_penalty([track]) -> frames
static int M_L_AssaultGetPenalty(lua_State *const L)
{
    lua_pushinteger(L, Gym_TrackManager_GetPenaltyFrames(M_GetTrack(L)));
    return 1;
}

// trxc.assault.get_target_penalty([track]) -> frames
static int M_L_AssaultGetTargetPenalty(lua_State *const L)
{
    lua_pushinteger(L, Gym_TrackManager_GetTargetPenaltyFrames(M_GetTrack(L)));
    return 1;
}

// trxc.assault.get_penalty_timer([track]) -> frames
static int M_L_AssaultGetPenaltyTimer(lua_State *const L)
{
    lua_pushinteger(L, Gym_TrackManager_GetPenaltyDisplayTimer(M_GetTrack(L)));
    return 1;
}

// trxc.assault.get_lap_time([track]) -> frames
static int M_L_AssaultGetLapTime(lua_State *const L)
{
    lua_pushinteger(L, Gym_TrackManager_GetLapTime(M_GetTrack(L)));
    return 1;
}

// trxc.assault.get_lap_timer([track]) -> frames
static int M_L_AssaultGetLapTimer(lua_State *const L)
{
    lua_pushinteger(L, Gym_TrackManager_GetLapTimeDisplayTimer(M_GetTrack(L)));
    return 1;
}

// trxc.assault.get_active_track() -> track or nil
static int M_L_AssaultGetActiveTrack(lua_State *const L)
{
    const GYM_TRACK_TYPE track = Gym_TrackManager_GetActiveTrackType();
    if (track == GYM_TRACK_NONE) {
        lua_pushnil(L);
    } else {
        lua_pushinteger(L, (lua_Integer)track);
    }
    return 1;
}

// trxc.assault.stats.record(time, [track]) -> bool
//
// The track comes second here and in remove: a record is about a time, and the
// time is what a script always says.
static int M_L_AssaultRecord(lua_State *const L)
{
    // Tested for what is allowed: NaN fails every comparison, so `time <= 0`
    // let it through to a cast that cannot hold it.
    const lua_Number time = luaL_checknumber(L, 1);
    if (!(time > 0.0 && time * LOGIC_FPS < (lua_Number)UINT32_MAX)) {
        return luaL_error(L, "time must be a positive number of seconds");
    }
    GYM_TRACK_STATS *const stats = M_CheckStats(L, M_GetTrackAt(L, 2));

    lua_pushboolean(L, M_StoreTime(stats, (float)time));
    return 1;
}

// trxc.assault.stats.remove(record_id, [track]) -> bool
static int M_L_AssaultRemoveRecord(lua_State *const L)
{
    // Lua's formatter has no %lld, and raised over the format rather than the
    // index.
    const lua_Integer index_1 = luaL_checkinteger(L, 1);
    if (index_1 < 1 || index_1 > MAX_ASSAULT_TIMES) {
        return luaL_error(
            L, "index out of range: %I (expected 1..%d)", index_1,
            MAX_ASSAULT_TIMES);
    }
    GYM_TRACK_STATS *const stats = M_CheckStats(L, M_GetTrackAt(L, 2));

    lua_pushboolean(L, M_RemoveTimeAtIndex(stats, (int32_t)index_1 - 1));
    return 1;
}

// trxc.assault.stats.list([track]) -> { { time=, attempt_num= }, ... }
static int M_L_AssaultListRecords(lua_State *const L)
{
    const GYM_TRACK_STATS *const stats = M_CheckStats(L, M_GetTrack(L));

    lua_newtable(L);
    int32_t out_idx = 1;

    for (int32_t i = 0; i < MAX_ASSAULT_TIMES; i++) {
        if (stats->entries[i].time == 0) {
            break;
        }

        lua_newtable(L);
        lua_pushnumber(
            L, (lua_Number)((float)stats->entries[i].time / LOGIC_FPS));
        lua_setfield(L, -2, "time");
        lua_pushinteger(L, (lua_Integer)stats->entries[i].attempt_num);
        lua_setfield(L, -2, "attempt_num");
        lua_seti(L, -2, out_idx);
        out_idx++;
    }

    return 1;
}

static const luaL_Reg m_Module[] = {
    { "start", M_L_AssaultStart },
    { "stop", M_L_AssaultStop },
    { "reset", M_L_AssaultReset },
    { "finish", M_L_AssaultFinish },
    { "is_running", M_L_AssaultIsRunning },
    { "is_visible", M_L_AssaultIsVisible },
    { "get_time", M_L_AssaultGetTime },
    { "get_best_time", M_L_AssaultGetBestTime },
    { "get_penalty", M_L_AssaultGetPenalty },
    { "get_target_penalty", M_L_AssaultGetTargetPenalty },
    { "get_penalty_timer", M_L_AssaultGetPenaltyTimer },
    { "get_lap_time", M_L_AssaultGetLapTime },
    { "get_lap_timer", M_L_AssaultGetLapTimer },
    { "get_active_track", M_L_AssaultGetActiveTrack },
    { nullptr, nullptr },
};

static const luaL_Reg m_Stats[] = {
    { "record", M_L_AssaultRecord },
    { "remove", M_L_AssaultRemoveRecord },
    { "list", M_L_AssaultListRecords },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "assault", m_Module);

    // The records are a group of their own on the module.
    LUA_GetModule(L, "assault");
    lua_newtable(L);
    luaL_setfuncs(L, m_Stats, 0);
    lua_setfield(L, -2, "stats");
    lua_pop(L, 1);
}

REGISTER_LUA_CAPI(.create = M_Create)
