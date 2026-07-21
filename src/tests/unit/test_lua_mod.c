// The mod surface. The assertions live in src/tests/unit/lua/mod.lua.
//
// The shell scans the mods once and hands out const pointers into a fixed list;
// the fake below is that list, plus the startup mod the args carry.

#include "lua_surface.h"

#include <trx/game/shell/common.h>
#include <trx/game/shell/mod.h>

static SHELL_MOD m_Mods[] = {
    {
        .name = "base",
        .title = "Base Game",
        .mod_type = MOD_BASE_GAME,
        .engine_version = 4,
        .base_mod = nullptr,
        .is_available = true,
        .is_valid = true,
    },
    {
        .name = "extra",
        .title = "Extra",
        .mod_type = MOD_CUSTOM,
        .engine_version = 4,
        .base_mod = "base",
        .is_available = true,
        .is_valid = false,
    },
};

static SHELL_ARGS m_Args = { .startup = { .mod = &m_Mods[0] } };

int32_t Shell_GetModCount(void)
{
    return 2;
}

const SHELL_MOD *Shell_GetMod(const int32_t index)
{
    if (index < 0 || index >= 2) {
        return nullptr;
    }
    return &m_Mods[index];
}

const SHELL_ARGS *Shell_GetArgs(void)
{
    return &m_Args;
}

static int M_FakeReset(lua_State *const L)
{
    return 0;
}

static int M_FakeCalls(lua_State *const L)
{
    lua_newtable(L);
    return 1;
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "mod",
        .tests = "mod",
        .seal = true,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
