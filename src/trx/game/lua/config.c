#include <trx/config/common.h>
#include <trx/config/option.h>
#include <trx/config/override.h>
#include <trx/game/lua/common.h>

#include <lauxlib.h>

static const CONFIG_OPTION *M_GetOption(lua_State *L, int32_t arg);
static void M_PushValue(lua_State *L, const CONFIG_OPTION *option);
static const char *M_ValueAsString(lua_State *L, int32_t arg);

static const CONFIG_OPTION *M_GetOption(lua_State *const L, const int32_t arg)
{
    const char *const key = luaL_checkstring(L, arg);
    const CONFIG_OPTION *const option = Config_GetOptionByPath(key);
    if (option == nullptr) {
        luaL_error(L, "Unknown option: %s", key);
    }
    return option;
}

// The option's declared type is what a script gets back: a bool reads as a bool
// and a number as a number. Colors and enums stay strings, which is what they
// are on the way in as well.
static void M_PushValue(lua_State *const L, const CONFIG_OPTION *const option)
{
    switch (option->type) {
    case COT_BOOL:
        lua_pushboolean(L, *(const bool *)option->target);
        return;
    case COT_INT32:
        lua_pushinteger(L, *(const int32_t *)option->target);
        return;
    case COT_ENUM:
        lua_pushinteger(L, *(const int *)option->target);
        return;
    case COT_FLOAT:
    case COT_FLOAT_PERCENT:
        lua_pushnumber(L, *(const float *)option->target);
        return;
    case COT_DOUBLE:
        lua_pushnumber(L, *(const double *)option->target);
        return;
    case COT_RGB888:
    case COT_STRING:
    case COT_DYNAMIC_ENUM:
        lua_pushstring(L, Config_GetOptionValueAsString(option, false));
        return;
    }
}

// Everything reaches the option through the one string parser, so a Lua value
// is spelled the way that parser reads it. A boolean has to be spelled out:
// lua's own conversion would make it "1".
static const char *M_ValueAsString(lua_State *const L, const int32_t arg)
{
    if (lua_isboolean(L, arg)) {
        return lua_toboolean(L, arg) ? "true" : "false";
    }
    if (lua_isnumber(L, arg)) {
        return lua_tostring(L, arg);
    }
    return luaL_checkstring(L, arg);
}

// trxc.config.get(key)
static int M_L_ConfigGet(lua_State *const L)
{
    M_PushValue(L, M_GetOption(L, 1));
    return 1;
}

// trxc.config.set(key, value)
static int M_L_ConfigSet(lua_State *const L)
{
    const CONFIG_OPTION *const option = M_GetOption(L, 1);
    const char *const new_value = M_ValueAsString(L, 2);
    if (!Config_SetOptionValueFromString(option, new_value)) {
        return luaL_error(
            L, "Failed to set option %s to %s", option->name, new_value);
    }
    Config_Update();
    return 0;
}

// trxc.config.override(key, value)
static int M_L_ConfigOverride(lua_State *const L)
{
    const CONFIG_OPTION *const option = M_GetOption(L, 1);
    const char *const new_value = M_ValueAsString(L, 2);
    if (!ConfigOverride_PushFromString(option, new_value)) {
        return luaL_error(
            L, "Failed to override option %s with %s", option->name, new_value);
    }
    return 0;
}

// trxc.config.restore(key) -> bool
static int M_L_ConfigRestore(lua_State *const L)
{
    const CONFIG_OPTION *const option = M_GetOption(L, 1);
    lua_pushboolean(L, ConfigOverride_Pop(option));
    return 1;
}

// trxc.config.is_overridden(key) -> bool
static int M_L_ConfigIsOverridden(lua_State *const L)
{
    const CONFIG_OPTION *const option = M_GetOption(L, 1);
    lua_pushboolean(L, ConfigOverride_IsOverridden(option));
    return 1;
}

// trxc.config.list()
static int M_L_ConfigList(lua_State *const L)
{
    lua_newtable(L);
    const CONFIG_OPTION *option = Config_GetOptionMap();
    while (option->name != nullptr) {
        M_PushValue(L, option);
        lua_setfield(L, -2, option->name);
        option++;
    }
    return 1;
}

void LUA_CreateConfig(lua_State *const L)
{
    lua_getglobal(L, "trxc");
    lua_newtable(L);
    lua_pushcfunction(L, M_L_ConfigGet);
    lua_setfield(L, -2, "get");
    lua_pushcfunction(L, M_L_ConfigSet);
    lua_setfield(L, -2, "set");
    lua_pushcfunction(L, M_L_ConfigOverride);
    lua_setfield(L, -2, "override");
    lua_pushcfunction(L, M_L_ConfigRestore);
    lua_setfield(L, -2, "restore");
    lua_pushcfunction(L, M_L_ConfigIsOverridden);
    lua_setfield(L, -2, "is_overridden");
    lua_pushcfunction(L, M_L_ConfigList);
    lua_setfield(L, -2, "list");
    lua_setfield(L, -2, "config");
    lua_pop(L, 1);
}
