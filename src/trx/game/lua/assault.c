#include <trx/game/game.h>
#include <trx/game/gym.h>
#include <trx/game/lua/common.h>

#include <lauxlib.h>

static void M_CheckAssaultTimerAvailable(lua_State *const L)
{
    if (!Gym_TrackManager_HasStats(GYM_TRACK_ASSAULT)) {
        luaL_error(L, "Assault timer unavailable");
    }
    if (!Game_IsInGym()) {
        luaL_error(L, "Assault timer is only available in gym levels");
    }
}

// trxc.assault.start()
static int M_L_AssaultStart(lua_State *const L)
{
    M_CheckAssaultTimerAvailable(L);
    Gym_TrackManager_Start(GYM_TRACK_ASSAULT);
    return 0;
}

// trxc.assault.stop()
static int M_L_AssaultStop(lua_State *const L)
{
    M_CheckAssaultTimerAvailable(L);
    Gym_TrackManager_Stop(GYM_TRACK_ASSAULT);
    return 0;
}

// trxc.assault.reset()
static int M_L_AssaultReset(lua_State *const L)
{
    M_CheckAssaultTimerAvailable(L);
    Gym_TrackManager_Reset(GYM_TRACK_ASSAULT);
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
