// The input system, reduced to what a surface test needs: every role is bound
// to one key on the keyboard, nothing is pressed until a test presses it, and
// rebinding does nothing. The real one reads devices and keeps the player's
// layouts.

#include <fakes/input.h>

#include <trx/game/input/raw.h>

#include <lauxlib.h>
#include <string.h>

static bool m_Pressed[INPUT_ROLE_NUMBER_OF] = {};
static bool m_Held[INPUT_ROLE_NUMBER_OF] = {};
static bool m_HeldOff[INPUT_ROLE_NUMBER_OF] = {};

static INPUT_ROLE M_CheckRole(lua_State *const L, const int arg)
{
    const lua_Integer role = luaL_checkinteger(L, arg);
    luaL_argcheck(L, role >= 0 && role < INPUT_ROLE_NUMBER_OF, arg, "role");
    return (INPUT_ROLE)role;
}

// fake.press(role) - the role goes down this tick. It stays held until
// fake.release_all, and reads as pressed until fake.hold moves it on.
static int M_L_Press(lua_State *const L)
{
    const INPUT_ROLE role = M_CheckRole(L, 1);
    m_Pressed[role] = true;
    m_Held[role] = true;
    m_HeldOff[role] = false;
    return 0;
}

// fake.hold(role, held) - the role is held, or let go, with no fresh press.
static int M_L_Hold(lua_State *const L)
{
    const INPUT_ROLE role = M_CheckRole(L, 1);
    m_Pressed[role] = false;
    m_Held[role] = lua_toboolean(L, 2);
    if (!m_Held[role]) {
        m_HeldOff[role] = false;
    }
    return 0;
}

// fake.release_all() - every role goes up, which also ends every hold-off.
static int M_L_ReleaseAll(lua_State *const L)
{
    FakeInput_Reset();
    return 0;
}

// fake.held_off(role) -> bool - whether a script used the press up.
static int M_L_HeldOff(lua_State *const L)
{
    lua_pushboolean(L, m_HeldOff[M_CheckRole(L, 1)]);
    return 1;
}

INPUT_STATE g_Input = {};
INPUT_STATE g_InputDB = {};

const char *Input_GetKeyName(
    const INPUT_BACKEND backend, const INPUT_LAYOUT layout,
    const INPUT_ROLE role, const int32_t slot)
{
    return slot == 0 ? "K" : nullptr;
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

bool Input_IsBackendEnabled(const INPUT_BACKEND backend)
{
    return backend == INPUT_BACKEND_KEYBOARD;
}

bool Input_IsRoleRebindable(const INPUT_ROLE role)
{
    return true;
}

bool Input_IsRoleUnbindable(const INPUT_ROLE role)
{
    return true;
}

bool Input_IsKeyConflicted(
    const INPUT_BACKEND backend, const INPUT_LAYOUT layout,
    const INPUT_ROLE role)
{
    return false;
}

bool Input_IsHeld(const INPUT_ROLE role)
{
    return m_Held[role] && !m_HeldOff[role];
}

bool Input_IsPressed(const INPUT_ROLE role)
{
    return m_Pressed[role] && !m_HeldOff[role];
}

void Input_HoldOffRole(const INPUT_ROLE role)
{
    m_HeldOff[role] = true;
}

void Input_HoldOffSkip(const INPUT_SKIP_CONTEXT context)
{
}

void Input_SuppressRole(const INPUT_ROLE role, const bool enabled)
{
}

bool Input_IsRoleSuppressed(const INPUT_ROLE role)
{
    return false;
}

void Input_ClearSuppressedRoles(void)
{
}

bool InputRaw_IsReserved(void)
{
    return false;
}

void InputRaw_SetScriptHold(const bool held)
{
}

bool InputRaw_IsHeldByScript(void)
{
    return false;
}

bool InputRaw_IsKeyHeld(const char *const key)
{
    return false;
}

bool InputRaw_IsKeyPressed(const char *const key)
{
    return false;
}

bool InputRaw_IsKeyKnown(const char *const key)
{
    return false;
}

bool InputRaw_IsButtonHeld(const char *const button)
{
    return false;
}

bool InputRaw_IsButtonPressed(const char *const button)
{
    return false;
}

bool InputRaw_IsButtonKnown(const char *const button)
{
    return false;
}

float InputRaw_GetAxis(const char *const axis)
{
    return 0.0f;
}

bool InputRaw_IsAxisKnown(const char *const axis)
{
    return false;
}

bool Input_ReadAndAssignRole(
    const INPUT_BACKEND backend, const INPUT_LAYOUT layout,
    const INPUT_ROLE role, const int32_t slot)
{
    return false;
}

void Input_UnassignRole(
    const INPUT_BACKEND backend, const INPUT_LAYOUT layout,
    const INPUT_ROLE role, const int32_t slot)
{
}

void Input_ResetLayout(const INPUT_BACKEND backend, const INPUT_LAYOUT layout)
{
}

void Input_EnterListenMode(void)
{
}

void Input_ExitListenMode(void)
{
}

bool Input_IsInListenMode(void)
{
    return false;
}

bool InputState_IsAnyPressed(const INPUT_STATE state)
{
    return false;
}

void FakeInput_Reset(void)
{
    memset(m_Pressed, 0, sizeof(m_Pressed));
    memset(m_Held, 0, sizeof(m_Held));
    memset(m_HeldOff, 0, sizeof(m_HeldOff));
}

void FakeInput_PushLua(lua_State *const L)
{
    lua_pushcfunction(L, M_L_Press);
    lua_setfield(L, -2, "press");
    lua_pushcfunction(L, M_L_Hold);
    lua_setfield(L, -2, "hold");
    lua_pushcfunction(L, M_L_ReleaseAll);
    lua_setfield(L, -2, "release_all");
    lua_pushcfunction(L, M_L_HeldOff);
    lua_setfield(L, -2, "held_off");
}
