// The input system, reduced to what a surface test needs: every role is bound
// to one key on the keyboard, nothing is pressed until a test presses it, and
// rebinding changes nothing but is counted. The real one reads devices and
// keeps the player's layouts.

#include <fakes/input.h>

#include <trx/game/input/raw.h>

#include <lauxlib.h>
#include <string.h>

static bool m_Pressed[INPUT_ROLE_NUMBER_OF] = {};
static bool m_Held[INPUT_ROLE_NUMBER_OF] = {};
static bool m_HeldOff[INPUT_ROLE_NUMBER_OF] = {};
static int32_t m_DownFor[INPUT_ROLE_NUMBER_OF] = {};
static bool m_ScriptHold = false;

// How many times the bindings were saved, for as long as the test runs.
static int32_t m_SavedCount = 0;

// Whether the on-screen controls are being picked from.
static bool m_Selecting = false;

// What else the bindings were asked to do, and whether the next read of a
// binding takes a key.
static int32_t m_BoundCount = 0;
static int32_t m_UnboundCount = 0;
static int32_t m_ResetCount = 0;
static bool m_BindNext = false;
static bool m_Listening = false;

// The menu keys that repeat while held, in ticks, as the real input repeats
// them after 0.4 seconds every 0.1 seconds, or every 0.2 for left and right.
static const struct {
    INPUT_ROLE role;
    int32_t delay;
    int32_t every;
} m_Repeats[] = {
    { INPUT_ROLE_MENU_UP, 12, 3 },   { INPUT_ROLE_MENU_DOWN, 12, 3 },
    { INPUT_ROLE_MENU_LEFT, 12, 6 }, { INPUT_ROLE_MENU_RIGHT, 12, 6 },
    { INPUT_ROLE_MENU_SKIP, 12, 3 }, { (INPUT_ROLE)-1, 0, 0 },
};

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

// fake.land_capture() - the next read of a binding takes a key.
static int M_L_LandCapture(lua_State *const L)
{
    m_BindNext = true;
    return 0;
}

// fake.bindings() -> table - how many times a binding was taken, cleared, a
// layout reset and the bindings saved, whether the game is listening for a
// key, and whether the on-screen controls are being picked from.
static int M_L_Bindings(lua_State *const L)
{
    lua_createtable(L, 0, 6);
    lua_pushinteger(L, m_BoundCount);
    lua_setfield(L, -2, "bound");
    lua_pushinteger(L, m_UnboundCount);
    lua_setfield(L, -2, "unbound");
    lua_pushinteger(L, m_ResetCount);
    lua_setfield(L, -2, "reset");
    lua_pushinteger(L, m_SavedCount);
    lua_setfield(L, -2, "saved");
    lua_pushboolean(L, m_Listening);
    lua_setfield(L, -2, "listening");
    lua_pushboolean(L, m_Selecting);
    lua_setfield(L, -2, "selecting");
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

// No screen to touch here.
bool Touch_HasHardwareSupport(void)
{
    return false;
}

void TouchOverlay_EnterSelectionMode(void)
{
    m_Selecting = true;
}

void TouchOverlay_ExitSelectionMode(void)
{
    m_Selecting = false;
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

bool Input_CanUnassignRole(
    const INPUT_BACKEND backend, const INPUT_LAYOUT layout,
    const INPUT_ROLE role, const int32_t slot)
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

bool Input_Grab_IsHeld(const INPUT_ROLE role)
{
    return m_ScriptHold && m_Held[role] && !m_HeldOff[role];
}

bool Input_Grab_IsPressed(const INPUT_ROLE role)
{
    if (!Input_Grab_IsHeld(role)) {
        return false;
    }
    const int32_t down_for = m_DownFor[role];
    if (down_for == 1) {
        return true;
    }
    for (int32_t i = 0; m_Repeats[i].role != (INPUT_ROLE)-1; i++) {
        if (m_Repeats[i].role == role) {
            return down_for > m_Repeats[i].delay
                && (down_for - m_Repeats[i].delay) % m_Repeats[i].every == 0;
        }
    }
    return false;
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
    m_ScriptHold = held;
}

bool InputRaw_IsHeldByScript(void)
{
    return m_ScriptHold;
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
    if (!m_BindNext) {
        return false;
    }
    m_BindNext = false;
    m_BoundCount++;
    return true;
}

void Input_UnassignRole(
    const INPUT_BACKEND backend, const INPUT_LAYOUT layout,
    const INPUT_ROLE role, const int32_t slot)
{
    m_UnboundCount++;
}

void Input_ResetLayout(const INPUT_BACKEND backend, const INPUT_LAYOUT layout)
{
    m_ResetCount++;
}

void Input_SaveBindings(void)
{
    m_SavedCount++;
}

void Input_EnterListenMode(void)
{
    m_Listening = true;
}

void Input_ExitListenMode(void)
{
    m_Listening = false;
}

bool Input_IsInListenMode(void)
{
    return m_Listening;
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
    memset(m_DownFor, 0, sizeof(m_DownFor));
}

void FakeInput_Tick(void)
{
    for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        m_DownFor[role] = m_Held[role] ? m_DownFor[role] + 1 : 0;
    }
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
    lua_pushcfunction(L, M_L_LandCapture);
    lua_setfield(L, -2, "land_capture");
    lua_pushcfunction(L, M_L_Bindings);
    lua_setfield(L, -2, "bindings");
}
