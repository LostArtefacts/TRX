// The argument parser surface. The assertions live in
// src/tests/unit/lua/argparse.lua; this stands up the world they run against.
//
// The matcher argparse leans on is the real one out of trx.strings, so what
// these assert is what the console does when a player types an argument.

#include "lua_surface.h"

static int M_FakeReset(lua_State *const L)
{
    return 0;
}

static int M_FakeCalls(lua_State *const L)
{
    lua_newtable(L);
    return 1;
}

static void M_PushFake(lua_State *const L)
{
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "argparse",
        .deps = { "strings", "locale", nullptr },
        .tests = "argparse",
        .push_fake = M_PushFake,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
