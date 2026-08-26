// The mod surface. The assertions live in mod.lua.
//
// The shell scans the mods once and hands out const pointers into a fixed list;
// the fake below is that list, plus the startup mod the args carry.

#include <harness/fake_calls.h>
#include <harness/lua_surface.h>

#include <trx/game/game_flow.h>
#include <trx/game/shell/args.h>
#include <trx/game/shell/mod.h>

#include <string.h>

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

static const char *m_RequestedMod;

static int M_FakeReset(lua_State *const L)
{
    FakeCalls_Reset();
    m_RequestedMod = nullptr;
    return 0;
}

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

const SHELL_MOD *Shell_GetModByName(const char *const name)
{
    for (int32_t i = 0; i < 2; i++) {
        if (strcmp(m_Mods[i].name, name) == 0) {
            return &m_Mods[i];
        }
    }
    return nullptr;
}

// A mod stands in for one that can be switched to when it is valid and is not
// a single level loaded on its own.
bool Shell_CanSwitchToMod(const SHELL_MOD *const mod)
{
    return mod != nullptr && mod->is_valid && mod->mod_type != MOD_DIRECT_LEVEL;
}

void Shell_RequestModSwitch(const char *const mod_name)
{
    m_RequestedMod = mod_name;
}

void GF_OverrideCommand(const GF_COMMAND command)
{
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .fake_reset = M_FakeReset,
        .module = "mod",
        .tests = "api/mod",
        .seal = true,
    };
    return LuaSurface_Run(&test);
}
