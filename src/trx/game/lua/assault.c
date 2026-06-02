#include <trx/config.h>
#include <trx/config/types.h>
#include <trx/game/const.h>
#include <trx/game/game.h>
#include <trx/game/gym.h>
#include <trx/game/lua/common.h>

#include <lauxlib.h>
#include <stdint.h>

static bool M_StoreAssaultTime(const float time)
{
    GYM_TRACK_STATS *const assault = &g_Config.profile.assault_stats;
    uint32_t logic_time = (uint32_t)(time * LOGIC_FPS);
    int32_t insert_idx = -1;

    for (int32_t i = 0; i < MAX_ASSAULT_TIMES; i++) {
        if (assault->entries[i].time == 0
            || logic_time < assault->entries[i].time) {
            insert_idx = i;
            break;
        }
    }
    if (insert_idx == -1) {
        return false;
    }

    for (int32_t i = MAX_ASSAULT_TIMES - 1; i > insert_idx; i--) {
        assault->entries[i] = assault->entries[i - 1];
    }

    assault->total_attempts++;
    assault->entries[insert_idx].time = logic_time;
    assault->entries[insert_idx].attempt_num = assault->total_attempts;
    Config_Update();
    return true;
}

static bool M_RemoveAssaultTimeAtIndex(const int32_t idx)
{
    GYM_TRACK_STATS *const assault = &g_Config.profile.assault_stats;
    if (idx < 0 || idx >= MAX_ASSAULT_TIMES) {
        return false;
    }
    if (assault->entries[idx].time == 0) {
        return false;
    }

    for (int32_t i = idx; i < MAX_ASSAULT_TIMES - 1; i++) {
        assault->entries[i] = assault->entries[i + 1];
    }

    assault->entries[MAX_ASSAULT_TIMES - 1].time = 0;
    assault->entries[MAX_ASSAULT_TIMES - 1].attempt_num = 0;
    Config_Update();
    return true;
}

static GYM_TRACK_TYPE M_GetTrack(lua_State *const L)
{
    const int32_t raw_track = (int32_t)luaL_optinteger(L, 1, GYM_TRACK_ASSAULT);

    switch (raw_track) {
    case GYM_TRACK_ASSAULT:
    case GYM_TRACK_QUAD:
        return (GYM_TRACK_TYPE)raw_track;
    default:
        luaL_error(L, "Invalid track (expected Track.COURSE or Track.QUAD)");
        return GYM_TRACK_ASSAULT;
    }
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
            L, "%s timer is only available in gym levels",
            M_GetTrackName(track));
    }

    if (!Gym_TrackManager_HasStats(track)) {
        luaL_error(L, "%s timer unavailable", M_GetTrackName(track));
    }
}

static int M_L_AssaultStart(lua_State *const L)
{
    const GYM_TRACK_TYPE track = M_GetTrack(L);
    M_CheckTimerAvailable(L, track);
    Gym_TrackManager_Start(track);
    return 0;
}

static int M_L_AssaultStop(lua_State *const L)
{
    const GYM_TRACK_TYPE track = M_GetTrack(L);
    M_CheckTimerAvailable(L, track);
    Gym_TrackManager_Stop(track);
    return 0;
}

static int M_L_AssaultReset(lua_State *const L)
{
    const GYM_TRACK_TYPE track = M_GetTrack(L);
    M_CheckTimerAvailable(L, track);
    Gym_TrackManager_Reset(track);
    return 0;
}

static int M_L_AssaultRecord(lua_State *const L)
{
    if (!Gym_TrackManager_HasStats(GYM_TRACK_ASSAULT)) {
        return luaL_error(L, "Assault stats unavailable");
    }

    const float time = (float)luaL_checknumber(L, 1);
    if (time <= 0.0f) {
        return luaL_error(L, "Time must be > 0");
    }

    const bool ok = M_StoreAssaultTime(time);
    lua_pushboolean(L, ok);
    return 1;
}

static int M_L_AssaultRemoveRecord(lua_State *const L)
{
    if (!Gym_TrackManager_HasStats(GYM_TRACK_ASSAULT)) {
        return luaL_error(L, "Assault stats unavailable");
    }

    const int64_t index_1 = luaL_checkinteger(L, 1);
    if (index_1 < 1 || index_1 > MAX_ASSAULT_TIMES) {
        return luaL_error(
            L, "Index out of range: %lld (expected 1..%d)", (long long)index_1,
            MAX_ASSAULT_TIMES);
    }

    const bool ok = M_RemoveAssaultTimeAtIndex(index_1 - 1);
    lua_pushboolean(L, ok);
    return 1;
}

static int M_L_AssaultListRecords(lua_State *const L)
{
    if (!Gym_TrackManager_HasStats(GYM_TRACK_ASSAULT)) {
        return luaL_error(L, "Assault stats unavailable");
    }

    const GYM_TRACK_STATS *const assault = &g_Config.profile.assault_stats;
    lua_newtable(L);
    int32_t out_idx = 1;

    for (int32_t i = 0; i < MAX_ASSAULT_TIMES; i++) {
        if (assault->entries[i].time == 0) {
            break;
        }

        lua_newtable(L);
        lua_pushnumber(
            L, (lua_Number)((float)assault->entries[i].time / LOGIC_FPS));
        lua_setfield(L, -2, "time");
        lua_pushinteger(L, (lua_Integer)assault->entries[i].attempt_num);
        lua_setfield(L, -2, "attempt_num");
        lua_seti(L, -2, out_idx);
        out_idx++;
    }

    return 1;
}

void LUA_CreateAssault(lua_State *const L)
{
    lua_getglobal(L, "trxc");
    lua_newtable(L);

    lua_pushcfunction(L, M_L_AssaultStart);
    lua_setfield(L, -2, "start");
    lua_pushcfunction(L, M_L_AssaultStop);
    lua_setfield(L, -2, "stop");
    lua_pushcfunction(L, M_L_AssaultReset);
    lua_setfield(L, -2, "reset");

    lua_newtable(L);
    lua_pushcfunction(L, M_L_AssaultRecord);
    lua_setfield(L, -2, "record");
    lua_pushcfunction(L, M_L_AssaultRemoveRecord);
    lua_setfield(L, -2, "remove");
    lua_pushcfunction(L, M_L_AssaultListRecords);
    lua_setfield(L, -2, "list");
    lua_setfield(L, -2, "stats");

    lua_newtable(L);
    lua_pushinteger(L, GYM_TRACK_ASSAULT);
    lua_setfield(L, -2, "COURSE");
    lua_pushinteger(L, GYM_TRACK_QUAD);
    lua_setfield(L, -2, "QUAD");
    lua_setfield(L, -2, "Track");

    lua_setfield(L, -2, "assault");
    lua_pop(L, 1);
}
