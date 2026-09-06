// Sets up the input surface. The assertions live in input.lua.

#include <harness/fake_calls.h>
#include <harness/lua_surface.h>

#include <trx/game/input.h>

#include <lauxlib.h>

// Sets the faked device state. Every role is bound to one key in one slot
// unless a test says otherwise.
static bool m_Bound = true;
static bool m_Held = false;
static bool m_Pressed = false;
static bool m_Rebindable = true;
static bool m_Unbindable = true;
static bool m_Conflicted = false;
static bool m_Listening = false;
static bool m_AnythingDown = true;
static bool m_AnythingHeld = false;

// Every test starts from a bound, quiet device.
static void M_Reset(void)
{
    m_Bound = true;
    m_Held = false;
    m_Pressed = false;
    m_Rebindable = true;
    m_Unbindable = true;
    m_Conflicted = false;
    m_Listening = false;
    m_AnythingDown = true;
    m_AnythingHeld = false;
}

FAKE_ON_RESET(M_Reset)

static int M_FakeSetBound(lua_State *const L)
{
    m_Bound = lua_toboolean(L, 1);
    return 0;
}

static int M_FakeSetHeld(lua_State *const L)
{
    m_Held = lua_toboolean(L, 1);
    return 0;
}

static int M_FakeSetPressed(lua_State *const L)
{
    m_Pressed = lua_toboolean(L, 1);
    return 0;
}

static int M_FakeSetRebindable(lua_State *const L)
{
    m_Rebindable = lua_toboolean(L, 1);
    return 0;
}

static int M_FakeSetUnbindable(lua_State *const L)
{
    m_Unbindable = lua_toboolean(L, 1);
    return 0;
}

static int M_FakeSetConflicted(lua_State *const L)
{
    m_Conflicted = lua_toboolean(L, 1);
    return 0;
}

static int M_FakeSetAnythingDown(lua_State *const L)
{
    m_AnythingDown = lua_toboolean(L, 1);
    return 0;
}

static int M_FakeSetAnythingHeld(lua_State *const L)
{
    m_AnythingHeld = lua_toboolean(L, 1);
    return 0;
}

static void M_PushFake(lua_State *const L)
{
    lua_pushcfunction(L, M_FakeSetBound);
    lua_setfield(L, -2, "set_bound");
    lua_pushcfunction(L, M_FakeSetHeld);
    lua_setfield(L, -2, "set_held");
    lua_pushcfunction(L, M_FakeSetPressed);
    lua_setfield(L, -2, "set_pressed");
    lua_pushcfunction(L, M_FakeSetRebindable);
    lua_setfield(L, -2, "set_rebindable");
    lua_pushcfunction(L, M_FakeSetUnbindable);
    lua_setfield(L, -2, "set_unbindable");
    lua_pushcfunction(L, M_FakeSetConflicted);
    lua_setfield(L, -2, "set_conflicted");
    lua_pushcfunction(L, M_FakeSetAnythingDown);
    lua_setfield(L, -2, "set_anything_down");
    lua_pushcfunction(L, M_FakeSetAnythingHeld);
    lua_setfield(L, -2, "set_anything_held");
}

INPUT_STATE g_Input;

bool InputState_IsAnyPressed(const INPUT_STATE state)
{
    return m_AnythingHeld;
}

bool Input_IsHeld(const INPUT_ROLE role)
{
    return m_Held;
}

bool Input_IsPressed(const INPUT_ROLE role)
{
    return m_Pressed;
}

void Input_HoldOffRole(const INPUT_ROLE role)
{
    FAKE_RECORD("hold_off", FV(role));
}

bool Input_IsBackendEnabled(const INPUT_BACKEND backend)
{
    return backend == INPUT_BACKEND_KEYBOARD;
}

bool Input_IsRoleRebindable(const INPUT_ROLE role)
{
    return m_Rebindable;
}

bool Input_IsRoleUnbindable(const INPUT_ROLE role)
{
    return m_Unbindable;
}

bool Input_IsKeyConflicted(
    const INPUT_BACKEND backend, const INPUT_LAYOUT layout,
    const INPUT_ROLE role)
{
    return m_Conflicted;
}

const char *Input_GetKeyName(
    const INPUT_BACKEND backend, const INPUT_LAYOUT layout,
    const INPUT_ROLE role, const int32_t slot)
{
    return m_Bound && slot == 0 ? "K" : nullptr;
}

const char *Input_GetRoleName(const INPUT_ROLE role)
{
    return "Jump";
}

const char *const *Input_GetLayoutNamePtr(const INPUT_LAYOUT layout)
{
    static const char *names[INPUT_LAYOUT_NUMBER_OF] = {
        "Default",
        "Custom 1",
        "Custom 2",
        "Custom 3",
    };
    return &names[layout];
}

bool Input_ReadAndAssignRole(
    const INPUT_BACKEND backend, const INPUT_LAYOUT layout,
    const INPUT_ROLE role, const int32_t slot)
{
    FAKE_RECORD("bind", FV(role), FV(slot), FV(backend), FV(layout));
    return m_AnythingDown;
}

void Input_UnassignRole(
    const INPUT_BACKEND backend, const INPUT_LAYOUT layout,
    const INPUT_ROLE role, const int32_t slot)
{
    FAKE_RECORD("unbind", FV(role), FV(slot), FV(backend), FV(layout));
}

void Input_ResetLayout(const INPUT_BACKEND backend, const INPUT_LAYOUT layout)
{
    FAKE_RECORD("reset_layout", FV(layout));
}

void Input_EnterListenMode(void)
{
    m_Listening = true;
}

void Input_ExitListenMode(void)
{
    FAKE_RECORD("exit_listen");
    m_Listening = false;
}

bool Input_IsInListenMode(void)
{
    return m_Listening;
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "input",
        .deps = { "signal", "events", nullptr },
        .tests = "api/input",
        .push_fake = M_PushFake,
    };
    return LuaSurface_Run(&test);
}
