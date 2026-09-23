// Sets up the input surface. The assertions live in input.lua.

#include <harness/fake_calls.h>
#include <harness/lua_surface.h>

#include <trx/game/input.h>
#include <trx/game/input/raw.h>

#include <lauxlib.h>
#include <string.h>

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
static bool m_Suppressed[INPUT_ROLE_NUMBER_OF] = {};

// The keyboard the raw reads see. Only these names are keys at all, so that a
// test can name one the layout does not carry.
static struct {
    const char *name;
    bool held;
    bool pressed;
} m_Keys[] = {
    { .name = "5" },      { .name = "a" },          { .name = "escape" },
    { .name = "return" }, { .name = "left shift" },
};

// The pad the raw reads see, named as SDL names buttons and axes.
static struct {
    const char *name;
    bool held;
    bool pressed;
} m_Buttons[] = {
    { .name = "a" },
    { .name = "dpup" },
    { .name = "leftshoulder" },
};

static struct {
    const char *name;
    float value;
} m_Axes[] = {
    { .name = "leftx" },
    { .name = "lefttrigger" },
};

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
    memset(m_Suppressed, 0, sizeof m_Suppressed);
    for (size_t i = 0; i < sizeof m_Keys / sizeof m_Keys[0]; i++) {
        m_Keys[i].held = false;
        m_Keys[i].pressed = false;
    }
    for (size_t i = 0; i < sizeof m_Buttons / sizeof m_Buttons[0]; i++) {
        m_Buttons[i].held = false;
        m_Buttons[i].pressed = false;
    }
    for (size_t i = 0; i < sizeof m_Axes / sizeof m_Axes[0]; i++) {
        m_Axes[i].value = 0.0f;
    }
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

static int M_FakeSetKeyHeld(lua_State *const L)
{
    const char *const name = luaL_checkstring(L, 1);
    for (size_t i = 0; i < sizeof m_Keys / sizeof m_Keys[0]; i++) {
        if (strcmp(m_Keys[i].name, name) == 0) {
            m_Keys[i].held = lua_toboolean(L, 2);
        }
    }
    return 0;
}

static int M_FakeSetKeyPressed(lua_State *const L)
{
    const char *const name = luaL_checkstring(L, 1);
    for (size_t i = 0; i < sizeof m_Keys / sizeof m_Keys[0]; i++) {
        if (strcmp(m_Keys[i].name, name) == 0) {
            m_Keys[i].pressed = lua_toboolean(L, 2);
        }
    }
    return 0;
}

static int M_FakeSetButtonHeld(lua_State *const L)
{
    const char *const name = luaL_checkstring(L, 1);
    for (size_t i = 0; i < sizeof m_Buttons / sizeof m_Buttons[0]; i++) {
        if (strcmp(m_Buttons[i].name, name) == 0) {
            m_Buttons[i].held = lua_toboolean(L, 2);
        }
    }
    return 0;
}

static int M_FakeSetButtonPressed(lua_State *const L)
{
    const char *const name = luaL_checkstring(L, 1);
    for (size_t i = 0; i < sizeof m_Buttons / sizeof m_Buttons[0]; i++) {
        if (strcmp(m_Buttons[i].name, name) == 0) {
            m_Buttons[i].pressed = lua_toboolean(L, 2);
        }
    }
    return 0;
}

static int M_FakeSetAxis(lua_State *const L)
{
    const char *const name = luaL_checkstring(L, 1);
    for (size_t i = 0; i < sizeof m_Axes / sizeof m_Axes[0]; i++) {
        if (strcmp(m_Axes[i].name, name) == 0) {
            m_Axes[i].value = (float)luaL_checknumber(L, 2);
        }
    }
    return 0;
}

static void M_PushFake(lua_State *const L)
{
    lua_pushcfunction(L, M_FakeSetButtonHeld);
    lua_setfield(L, -2, "set_button_held");
    lua_pushcfunction(L, M_FakeSetButtonPressed);
    lua_setfield(L, -2, "set_button_pressed");
    lua_pushcfunction(L, M_FakeSetAxis);
    lua_setfield(L, -2, "set_axis");
    lua_pushcfunction(L, M_FakeSetKeyHeld);
    lua_setfield(L, -2, "set_key_held");
    lua_pushcfunction(L, M_FakeSetKeyPressed);
    lua_setfield(L, -2, "set_key_pressed");
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
    return m_Held && !m_Suppressed[role];
}

bool Input_IsPressed(const INPUT_ROLE role)
{
    return m_Pressed && !m_Suppressed[role];
}

void Input_HoldOffRole(const INPUT_ROLE role)
{
    FAKE_RECORD("hold_off", FV(role));
}

void Input_SuppressRole(const INPUT_ROLE role, const bool enabled)
{
    m_Suppressed[role] = enabled;
    FAKE_RECORD("suppress", FV(role), FV(enabled));
}

bool Input_IsRoleSuppressed(const INPUT_ROLE role)
{
    return m_Suppressed[role];
}

void Input_ClearSuppressedRoles(void)
{
    memset(m_Suppressed, 0, sizeof m_Suppressed);
    FAKE_RECORD("clear_suppressed");
}

bool InputRaw_IsReserved(void)
{
    return m_Listening;
}

bool InputRaw_IsKeyHeld(const char *const key)
{
    if (InputRaw_IsReserved()) {
        return false;
    }
    for (size_t i = 0; i < sizeof m_Keys / sizeof m_Keys[0]; i++) {
        if (strcmp(m_Keys[i].name, key) == 0) {
            return m_Keys[i].held;
        }
    }
    return false;
}

bool InputRaw_IsKeyPressed(const char *const key)
{
    if (InputRaw_IsReserved()) {
        return false;
    }
    for (size_t i = 0; i < sizeof m_Keys / sizeof m_Keys[0]; i++) {
        if (strcmp(m_Keys[i].name, key) == 0) {
            return m_Keys[i].pressed;
        }
    }
    return false;
}

bool InputRaw_IsKeyKnown(const char *const key)
{
    for (size_t i = 0; i < sizeof m_Keys / sizeof m_Keys[0]; i++) {
        if (strcmp(m_Keys[i].name, key) == 0) {
            return true;
        }
    }
    return false;
}

bool InputRaw_IsButtonHeld(const char *const button)
{
    if (InputRaw_IsReserved()) {
        return false;
    }
    for (size_t i = 0; i < sizeof m_Buttons / sizeof m_Buttons[0]; i++) {
        if (strcmp(m_Buttons[i].name, button) == 0) {
            return m_Buttons[i].held;
        }
    }
    return false;
}

bool InputRaw_IsButtonPressed(const char *const button)
{
    if (InputRaw_IsReserved()) {
        return false;
    }
    for (size_t i = 0; i < sizeof m_Buttons / sizeof m_Buttons[0]; i++) {
        if (strcmp(m_Buttons[i].name, button) == 0) {
            return m_Buttons[i].pressed;
        }
    }
    return false;
}

bool InputRaw_IsButtonKnown(const char *const button)
{
    for (size_t i = 0; i < sizeof m_Buttons / sizeof m_Buttons[0]; i++) {
        if (strcmp(m_Buttons[i].name, button) == 0) {
            return true;
        }
    }
    return false;
}

float InputRaw_GetAxis(const char *const axis)
{
    if (InputRaw_IsReserved()) {
        return 0.0f;
    }
    for (size_t i = 0; i < sizeof m_Axes / sizeof m_Axes[0]; i++) {
        if (strcmp(m_Axes[i].name, axis) == 0) {
            return m_Axes[i].value;
        }
    }
    return 0.0f;
}

bool InputRaw_IsAxisKnown(const char *const axis)
{
    for (size_t i = 0; i < sizeof m_Axes / sizeof m_Axes[0]; i++) {
        if (strcmp(m_Axes[i].name, axis) == 0) {
            return true;
        }
    }
    return false;
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
