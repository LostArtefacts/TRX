#include <trx/game/lua/common.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>
#include <trx/game/sound/common.h>

#include <lauxlib.h>

// trxc.sound.is_available(id)
static int M_L_SoundIsAvailable(lua_State *const L)
{
    const SAMPLE_ID id = (SAMPLE_ID)luaL_checkinteger(L, 1);
    lua_pushboolean(L, Sound_IsAvailable_Direct(id));
    return 1;
}

// trxc.sound.play(id[, opts])
static int M_L_SoundPlay(lua_State *const L)
{
    const SAMPLE_ID id = (SAMPLE_ID)luaL_checkinteger(L, 1);
    if (!Sound_IsAvailable_Direct(id)) {
        return luaL_error(L, "invalid sound track: %d", (int)id);
    }
    XYZ_32 pos;
    const XYZ_32 *pos_ptr = nullptr;
    if (lua_gettop(L) >= 2 && lua_istable(L, 2)) {
        if (lua_getfield(L, 2, "pos") == LUA_TTABLE) {
            // The blame is the options table, which is what the script wrote.
            pos = LUA_CheckXYZAt(L, -1, 2);
            pos_ptr = &pos;
        }
        lua_pop(L, 1);
    }
    Sound_Effect_Direct(id, pos_ptr, SPM_ALWAYS | SPM_STATIC_POS);
    return 0;
}

// trxc.sound.stop(id)
static int M_L_SoundStop(lua_State *const L)
{
    const SAMPLE_ID id = (SAMPLE_ID)luaL_checkinteger(L, 1);
    Sound_StopEffect_Direct(id);
    return 0;
}

// trxc.sound.stop_all()
static int M_L_SoundStopAll(lua_State *const L)
{
    Sound_StopAll();
    return 0;
}

static const luaL_Reg m_Module[] = {
    { "is_available", M_L_SoundIsAvailable },
    { "play", M_L_SoundPlay },
    { "stop", M_L_SoundStop },
    { "stop_all", M_L_SoundStopAll },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "sound", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
