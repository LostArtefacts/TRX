#include <trx/game/game.h>
#include <trx/game/gym.h>
#include <trx/game/lua/common.h>

#include <lauxlib.h>
#include <string.h>


static void M_CheckTimerAvailable(
    lua_State *const L,
    const GYM_TRACK_TYPE track,
    const char *const name)
{
    if (!Gym_TrackManager_HasStats(track)) {
        luaL_error(L, "%s timer unavailable", name);
    }

    if (!Game_IsInGym()) {
        luaL_error(L, "%s timer is only available in gym levels", name);
    }
}


static GYM_TRACK_TYPE M_GetTrack(lua_State *L)
{
    const char *mode = luaL_optstring(L, 1, "course");

    if (strcmp(mode, "quad") == 0) {
        return GYM_TRACK_QUAD;
    }

    return GYM_TRACK_ASSAULT;
}

static const char *M_GetTrackName(lua_State *L)
{
    const char *mode = luaL_optstring(L, 1, "course");

    if (strcmp(mode, "quad") == 0) {
        return "Quad";
    }

    return "Course";
}

static int M_L_AssaultStart(lua_State *L)
{
    GYM_TRACK_TYPE track = M_GetTrack(L);
    const char *name = M_GetTrackName(L);

    M_CheckTimerAvailable(L, track, name);
    Gym_TrackManager_Start(track);

    return 0;
}

static int M_L_AssaultStop(lua_State *L)
{
    GYM_TRACK_TYPE track = M_GetTrack(L);
    const char *name = M_GetTrackName(L);

    M_CheckTimerAvailable(L, track, name);
    Gym_TrackManager_Stop(track);

    return 0;
}

static int M_L_AssaultReset(lua_State *L)
{
    GYM_TRACK_TYPE track = M_GetTrack(L);
    const char *name = M_GetTrackName(L);

    M_CheckTimerAvailable(L, track, name);
    Gym_TrackManager_Reset(track);

    return 0;
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

    lua_setfield(L, -2, "assault");

    lua_pop(L, 1);
}
