#include <trx/game/cutseq.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>

#include <lauxlib.h>

#define M_NO_CUTSCENE (-1)
#define M_NO_FRAME (-1)

// A cutscene a script asks to play has to be one this game can play, or the
// call would do nothing and say nothing about why.
static int32_t M_CheckPlayableNum(lua_State *const L, const int32_t arg)
{
    const lua_Integer num = luaL_checkinteger(L, arg);
    if (!CutSeq_IsAvailable()) {
        luaL_error(L, "this game has no cutscenes");
    }
    luaL_argcheck(
        L, num >= 0 && num < CutSeq_GetCount(), arg, "unknown cutscene");
    return (int32_t)num;
}

// The played-once memory covers every number a trigger may carry, including
// ones the pak holds no scene for, so it takes a wider range than play does.
static int32_t M_CheckTriggerNum(lua_State *const L, const int32_t arg)
{
    const lua_Integer num = luaL_checkinteger(L, arg);
    luaL_argcheck(
        L, num >= 0 && num < CUTSEQ_MAX_TRIGGERS, arg, "unknown cutscene");
    return (int32_t)num;
}

// trxc.cutscenes.play(num, fade)
static int M_L_CutscenesPlay(lua_State *const L)
{
    const int32_t num = M_CheckPlayableNum(L, 1);
    const bool fade_out = lua_isnoneornil(L, 2) || lua_toboolean(L, 2);
    CutSeq_Request(num, fade_out);
    return 0;
}

// trxc.cutscenes.get_current() → int or nil
static int M_L_CutscenesGetCurrent(lua_State *const L)
{
    LUA_PushOptIndex(L, CutSeq_GetCurrent(), M_NO_CUTSCENE);
    return 1;
}

// trxc.cutscenes.get_frame_num() → int or nil
static int M_L_CutscenesGetFrameNum(lua_State *const L)
{
    LUA_PushOptIndex(L, CutSeq_GetFrame(), M_NO_FRAME);
    return 1;
}

// trxc.cutscenes.is_playing() → bool
static int M_L_CutscenesIsPlaying(lua_State *const L)
{
    lua_pushboolean(L, CutSeq_IsPlaying());
    return 1;
}

// trxc.cutscenes.is_played(num) → bool
static int M_L_CutscenesIsPlayed(lua_State *const L)
{
    lua_pushboolean(L, CutSeq_IsPlayed(M_CheckTriggerNum(L, 1)));
    return 1;
}

// trxc.cutscenes.set_played(num, played)
static int M_L_CutscenesSetPlayed(lua_State *const L)
{
    const int32_t num = M_CheckTriggerNum(L, 1);
    CutSeq_SetPlayed(num, lua_toboolean(L, 2));
    return 0;
}

// trxc.cutscenes.forget_played()
static int M_L_CutscenesForgetPlayed(lua_State *const L)
{
    CutSeq_SetPlayedMask(0);
    return 0;
}

// trxc.cutscenes.set_lara_return({x,y,z}, rot)
static int M_L_CutscenesSetLaraReturn(lua_State *const L)
{
    const XYZ_32 pos = LUA_CheckXYZ(L, 1);
    int16_t rot = 0;
    if (!lua_isnoneornil(L, 2)) {
        rot = (int16_t)luaL_checkinteger(L, 2);
    }
    CutSeq_SetLaraReturn(pos, rot);
    return 0;
}

// trxc.cutscenes.get_fov() → int
static int M_L_CutscenesGetFOV(lua_State *const L)
{
    lua_pushinteger(L, CutSeq_GetFOV());
    return 1;
}

// trxc.cutscenes.set_fov(fov)
static int M_L_CutscenesSetFOV(lua_State *const L)
{
    CutSeq_SetFOV((int32_t)luaL_checkinteger(L, 1));
    return 0;
}

// trxc.cutscenes.get_letterbox() → number
static int M_L_CutscenesGetLetterbox(lua_State *const L)
{
    lua_pushnumber(L, CutSeq_GetLetterbox());
    return 1;
}

// trxc.cutscenes.set_letterbox(ratio)
static int M_L_CutscenesSetLetterbox(lua_State *const L)
{
    CutSeq_SetLetterbox((float)luaL_checknumber(L, 1));
    return 0;
}

static const luaL_Reg m_Module[] = {
    { "play", M_L_CutscenesPlay },
    { "get_current", M_L_CutscenesGetCurrent },
    { "get_frame_num", M_L_CutscenesGetFrameNum },
    { "is_playing", M_L_CutscenesIsPlaying },
    { "is_played", M_L_CutscenesIsPlayed },
    { "set_played", M_L_CutscenesSetPlayed },
    { "forget_played", M_L_CutscenesForgetPlayed },
    { "set_lara_return", M_L_CutscenesSetLaraReturn },
    { "get_fov", M_L_CutscenesGetFOV },
    { "set_fov", M_L_CutscenesSetFOV },
    { "get_letterbox", M_L_CutscenesGetLetterbox },
    { "set_letterbox", M_L_CutscenesSetLetterbox },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "cutscenes", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
