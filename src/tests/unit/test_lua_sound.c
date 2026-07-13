// The sound surface. The assertions live in src/tests/unit/lua/sound.lua; this
// stands up the world they run against.

#include "fake_engine_sound.h"
#include "lua_surface.h"

extern void LUA_CreateSound(lua_State *L);

static void M_SetUpTRXC(lua_State *const L)
{
    LUA_CreateSound(L);
}

static int M_FakeReset(lua_State *const L)
{
    FakeSound_Reset();
    return 0;
}

static int M_FakeCalls(lua_State *const L)
{
    lua_newtable(L);
    lua_pushinteger(L, g_FakeSoundCalls.play_count);
    lua_setfield(L, -2, "play_count");
    lua_pushinteger(L, g_FakeSoundCalls.last_sample);
    lua_setfield(L, -2, "last_sample");
    lua_pushboolean(L, g_FakeSoundCalls.had_pos);
    lua_setfield(L, -2, "had_pos");
    lua_pushinteger(L, g_FakeSoundCalls.last_x);
    lua_setfield(L, -2, "last_x");
    lua_pushinteger(L, g_FakeSoundCalls.last_y);
    lua_setfield(L, -2, "last_y");
    lua_pushinteger(L, g_FakeSoundCalls.last_z);
    lua_setfield(L, -2, "last_z");
    lua_pushinteger(L, g_FakeSoundCalls.stop_count);
    lua_setfield(L, -2, "stop_count");
    lua_pushinteger(L, g_FakeSoundCalls.last_stopped_sample);
    lua_setfield(L, -2, "last_stopped_sample");
    lua_pushinteger(L, g_FakeSoundCalls.stop_all_count);
    lua_setfield(L, -2, "stop_all_count");
    return 1;
}

static void M_PushFake(lua_State *const L)
{
    lua_pushinteger(L, FAKE_SAMPLE);
    lua_setfield(L, -2, "SAMPLE");
    lua_pushinteger(L, FAKE_MISSING_SAMPLE);
    lua_setfield(L, -2, "MISSING_SAMPLE");
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "sound",
        .tests = "sound",
        .setup_trxc = M_SetUpTRXC,
        .push_fake = M_PushFake,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
