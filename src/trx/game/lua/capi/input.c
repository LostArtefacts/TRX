#include <trx/config.h>
#include <trx/game/input.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>

#include <lauxlib.h>

static INPUT_ROLE M_CheckRole(lua_State *const L, const int arg)
{
    const lua_Integer role = luaL_checkinteger(L, arg);
    if (role < 0 || role >= INPUT_ROLE_NUMBER_OF) {
        luaL_error(L, "unknown input role");
    }
    return (INPUT_ROLE)role;
}

// Converts a Lua slot number to the zero-based slot used by the input code.
static int32_t M_OptSlot(lua_State *const L, const int arg)
{
    const lua_Integer slot = luaL_optinteger(L, arg, 1);
    if (slot < 1 || slot > INPUT_BINDING_SLOTS) {
        luaL_error(L, "slot must be between 1 and %d", INPUT_BINDING_SLOTS);
    }
    return (int32_t)(slot - 1);
}

static INPUT_BACKEND M_CheckBackend(lua_State *const L, const int arg)
{
    const lua_Integer backend = luaL_checkinteger(L, arg);
    if (backend < 0 || backend >= INPUT_BACKEND_NUMBER_OF) {
        luaL_error(L, "unknown input backend");
    }
    return (INPUT_BACKEND)backend;
}

static INPUT_BACKEND M_OptBackend(lua_State *const L, const int arg)
{
    if (lua_isnoneornil(L, arg)) {
        return g_Config.input.backend;
    }
    return M_CheckBackend(L, arg);
}

static INPUT_LAYOUT M_OptLayout(
    lua_State *const L, const int arg, const INPUT_BACKEND backend)
{
    if (lua_isnoneornil(L, arg)) {
        return (INPUT_LAYOUT)g_Config.input.layout[backend];
    }
    const lua_Integer layout = luaL_checkinteger(L, arg);
    if (layout < 0 || layout >= INPUT_LAYOUT_NUMBER_OF) {
        luaL_error(L, "unknown input layout");
    }
    return (INPUT_LAYOUT)layout;
}

// Checks that a layout can be changed. The default layout is read-only because
// custom layouts reset from it.
static INPUT_LAYOUT M_CheckCustomLayout(
    lua_State *const L, const int arg, const INPUT_BACKEND backend)
{
    const INPUT_LAYOUT layout = M_OptLayout(L, arg, backend);
    if (layout == INPUT_LAYOUT_DEFAULT) {
        luaL_error(L, "the default layout cannot be changed");
    }
    return layout;
}

// trxc.input.backend() -> int
static int M_L_InputBackend(lua_State *const L)
{
    lua_pushinteger(L, g_Config.input.backend);
    return 1;
}

// trxc.input.layout([backend]) -> int
static int M_L_InputLayout(lua_State *const L)
{
    const INPUT_BACKEND backend = M_OptBackend(L, 1);
    lua_pushinteger(L, g_Config.input.layout[backend]);
    return 1;
}

// trxc.input.layout_name([layout]) -> string
static int M_L_InputLayoutName(lua_State *const L)
{
    const INPUT_LAYOUT layout = M_OptLayout(L, 1, g_Config.input.backend);
    const char *const *const name = Input_GetLayoutNamePtr(layout);
    lua_pushstring(L, name == nullptr ? "" : *name);
    return 1;
}

// trxc.input.is_backend_enabled(backend) -> bool
static int M_L_InputIsBackendEnabled(lua_State *const L)
{
    lua_pushboolean(L, Input_IsBackendEnabled(M_CheckBackend(L, 1)));
    return 1;
}

// trxc.input.is_anything_held() -> bool
static int M_L_InputIsAnythingHeld(lua_State *const L)
{
    lua_pushboolean(L, InputState_IsAnyPressed(g_Input));
    return 1;
}

// trxc.input.is_held(role) -> bool
static int M_L_InputIsHeld(lua_State *const L)
{
    lua_pushboolean(L, Input_IsHeld(M_CheckRole(L, 1)));
    return 1;
}

// trxc.input.is_pressed(role) -> bool
static int M_L_InputIsPressed(lua_State *const L)
{
    lua_pushboolean(L, Input_IsPressed(M_CheckRole(L, 1)));
    return 1;
}

// trxc.input.hold_off(role)
static int M_L_InputHoldOff(lua_State *const L)
{
    Input_HoldOffRole(M_CheckRole(L, 1));
    return 0;
}

// trxc.input.role_name(role) -> string
static int M_L_InputRoleName(lua_State *const L)
{
    lua_pushstring(L, Input_GetRoleName(M_CheckRole(L, 1)));
    return 1;
}

// trxc.input.key_name(role, [slot], [backend], [layout]) -> string|nil
static int M_L_InputKeyName(lua_State *const L)
{
    const INPUT_ROLE role = M_CheckRole(L, 1);
    const int32_t slot = M_OptSlot(L, 2);
    const INPUT_BACKEND backend = M_OptBackend(L, 3);
    const INPUT_LAYOUT layout = M_OptLayout(L, 4, backend);
    const char *const name = Input_GetKeyName(backend, layout, role, slot);
    if (name == nullptr) {
        lua_pushnil(L);
    } else {
        lua_pushstring(L, name);
    }
    return 1;
}

// trxc.input.is_rebindable(role) -> bool
static int M_L_InputIsRebindable(lua_State *const L)
{
    lua_pushboolean(L, Input_IsRoleRebindable(M_CheckRole(L, 1)));
    return 1;
}

// trxc.input.is_unbindable(role) -> bool
static int M_L_InputIsUnbindable(lua_State *const L)
{
    lua_pushboolean(L, Input_IsRoleUnbindable(M_CheckRole(L, 1)));
    return 1;
}

// trxc.input.is_conflicted(role, [backend], [layout]) -> bool
static int M_L_InputIsConflicted(lua_State *const L)
{
    const INPUT_ROLE role = M_CheckRole(L, 1);
    const INPUT_BACKEND backend = M_OptBackend(L, 2);
    const INPUT_LAYOUT layout = M_OptLayout(L, 3, backend);
    lua_pushboolean(L, Input_IsKeyConflicted(backend, layout, role));
    return 1;
}

// trxc.input.bind_pressed(role, [slot], [backend], [layout]) -> bool
static int M_L_InputBindPressed(lua_State *const L)
{
    const INPUT_ROLE role = M_CheckRole(L, 1);
    const int32_t slot = M_OptSlot(L, 2);
    const INPUT_BACKEND backend = M_OptBackend(L, 3);
    const INPUT_LAYOUT layout = M_CheckCustomLayout(L, 4, backend);
    if (!Input_IsRoleRebindable(role)) {
        return luaL_error(L, "the role cannot be rebound");
    }
    lua_pushboolean(L, Input_ReadAndAssignRole(backend, layout, role, slot));
    return 1;
}

// trxc.input.unbind(role, [slot], [backend], [layout])
static int M_L_InputUnbind(lua_State *const L)
{
    const INPUT_ROLE role = M_CheckRole(L, 1);
    const int32_t slot = M_OptSlot(L, 2);
    const INPUT_BACKEND backend = M_OptBackend(L, 3);
    const INPUT_LAYOUT layout = M_CheckCustomLayout(L, 4, backend);
    if (!Input_IsRoleUnbindable(role)) {
        return luaL_error(L, "the role cannot be unbound");
    }
    Input_UnassignRole(backend, layout, role, slot);
    return 0;
}

// trxc.input.reset_layout([backend], [layout])
static int M_L_InputResetLayout(lua_State *const L)
{
    const INPUT_BACKEND backend = M_OptBackend(L, 1);
    const INPUT_LAYOUT layout = M_CheckCustomLayout(L, 2, backend);
    Input_ResetLayout(backend, layout);
    return 0;
}

// trxc.input.listen(enabled)
static int M_L_InputListen(lua_State *const L)
{
    luaL_checktype(L, 1, LUA_TBOOLEAN);
    if (lua_toboolean(L, 1)) {
        Input_EnterListenMode();
    } else if (Input_IsInListenMode()) {
        // Leaving listen mode resets the debounce, so turning off what is
        // already off would swallow a press the game has not read yet.
        Input_ExitListenMode();
    }
    return 0;
}

// trxc.input.is_listening() -> bool
static int M_L_InputIsListening(lua_State *const L)
{
    lua_pushboolean(L, Input_IsInListenMode());
    return 1;
}

static const luaL_Reg m_Module[] = {
    { "backend", M_L_InputBackend },
    { "bind_pressed", M_L_InputBindPressed },
    { "hold_off", M_L_InputHoldOff },
    { "is_anything_held", M_L_InputIsAnythingHeld },
    { "is_backend_enabled", M_L_InputIsBackendEnabled },
    { "is_conflicted", M_L_InputIsConflicted },
    { "is_held", M_L_InputIsHeld },
    { "is_listening", M_L_InputIsListening },
    { "is_pressed", M_L_InputIsPressed },
    { "is_rebindable", M_L_InputIsRebindable },
    { "is_unbindable", M_L_InputIsUnbindable },
    { "key_name", M_L_InputKeyName },
    { "layout", M_L_InputLayout },
    { "layout_name", M_L_InputLayoutName },
    { "listen", M_L_InputListen },
    { "reset_layout", M_L_InputResetLayout },
    { "role_name", M_L_InputRoleName },
    { "unbind", M_L_InputUnbind },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "input", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
