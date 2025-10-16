#include "game/lua/common.h"
#include "game/music/common.h"

#include <lauxlib.h>

// trx.music.get_track()
static int M_L_MusicGetTrack(lua_State *const L)
{
    const MUSIC_ID track = Music_GetCurrentPlayingTrack();
    if (track < 0) {
        lua_pushnil(L);
    } else {
        lua_pushinteger(L, (lua_Integer)track);
    }
    return 1;
}

// trx.music.play_track(id[, opts])
static int M_L_MusicPlayTrack(lua_State *const L)
{
    const lua_Integer id = luaL_checkinteger(L, 1);
    MUSIC_PLAY_MODE mode = MPM_ALWAYS;
    if (lua_gettop(L) >= 2 && lua_istable(L, 2)) {
        lua_getfield(L, 2, "mode");
        mode = (MUSIC_PLAY_MODE)luaL_optinteger(L, -1, mode);
        lua_pop(L, 1);
    }
    if (!Music_Play_Direct((MUSIC_ID)id, mode)) {
        return luaL_error(L, "invalid music track or mode: %d", (int32_t)id);
    }
    return 0;
}

// trx.music.pause()
static int M_L_MusicPause(lua_State *const L)
{
    Music_Pause();
    return 0;
}

// trx.music.unpause()
static int M_L_MusicUnpause(lua_State *const L)
{
    Music_Unpause();
    return 0;
}

// trx.music.stop()
static int M_L_MusicStop(lua_State *const L)
{
    Music_Stop();
    return 0;
}

void LUA_CreateMusic(lua_State *const L)
{
    lua_getglobal(L, "trx");
    lua_newtable(L);
    lua_pushinteger(L, MPM_ALWAYS);
    lua_setfield(L, -2, "MPM_ALWAYS");
    lua_pushinteger(L, MPM_LOOPED);
    lua_setfield(L, -2, "MPM_LOOPED");
    lua_pushinteger(L, MPM_DELAYED);
    lua_setfield(L, -2, "MPM_DELAYED");
    lua_pushinteger(L, MPM_TRACKED);
    lua_setfield(L, -2, "MPM_TRACKED");
    lua_pushcfunction(L, M_L_MusicGetTrack);
    lua_setfield(L, -2, "get_track");
    lua_pushcfunction(L, M_L_MusicPlayTrack);
    lua_setfield(L, -2, "play_track");
    lua_pushcfunction(L, M_L_MusicPlayTrack);
    lua_setfield(L, -2, "play");
    lua_pushcfunction(L, M_L_MusicPause);
    lua_setfield(L, -2, "pause");
    lua_pushcfunction(L, M_L_MusicUnpause);
    lua_setfield(L, -2, "unpause");
    lua_pushcfunction(L, M_L_MusicStop);
    lua_setfield(L, -2, "stop");
    lua_setfield(L, -2, "music");
    lua_pop(L, 1);
}
