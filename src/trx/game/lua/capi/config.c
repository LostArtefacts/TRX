#include <trx/config/common.h>
#include <trx/config/option.h>
#include <trx/config/override.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>

#include <lauxlib.h>

static const CONFIG_OPTION *M_GetOption(lua_State *const L, const int32_t arg)
{
    const char *const key = luaL_checkstring(L, arg);
    const CONFIG_OPTION *const option = Config_GetOptionByPath(key);
    if (option == nullptr) {
        luaL_error(L, "unknown option: %s", key);
    }
    return option;
}

// The option's declared type is what a script gets back: a bool reads as a bool
// and a number as a number. Colors and enums stay strings, which is what they
// are on the way in as well.
static void M_PushValue(lua_State *const L, const CONFIG_OPTION *const option)
{
    // Colours, enums, strings and dynamic enums read back as their name or
    // display string, which is also how a script gives them on the way in.
    if (option->type == TVT_RGB_888 || option->type == TVT_ENUM
        || option->type == TVT_STRING || option->type == TVT_DYNAMIC_ENUM) {
        lua_pushstring(L, Config_GetOptionValueAsString(option, false));
        return;
    }
    TRX_VALUE value;
    Value_ReadPtr(option->type, option->target, &value);
    LUA_PushValue(L, &value);
}

// A value is handed to the config string parser, so it is spelled the way that
// parser reads it. A boolean has to be spelled out: Lua's own conversion would
// make it "1".
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

// trxc.config.set(key, value, force?)
static int M_L_ConfigSet(lua_State *const L)
{
    const CONFIG_OPTION *const option = M_GetOption(L, 1);
    const char *const new_value = M_ValueAsString(L, 2);
    const bool force = lua_toboolean(L, 3);
    if (!(force ? Config_SetOptionValueFromStringForce(option, new_value)
                : Config_SetOptionValueFromString(option, new_value))) {
        return luaL_error(
            L, "failed to set option %s to %s", option->name, new_value);
    }
    Config_Update();
    return 0;
}

// trxc.config.reset(key, force?) -> bool
static int M_L_ConfigReset(lua_State *const L)
{
    const CONFIG_OPTION *const option = M_GetOption(L, 1);
    const bool force = lua_toboolean(L, 2);
    const bool changed = force
        ? Config_RestoreOptionDefaultForce(option->target)
        : Config_RestoreOptionDefault(option->target);
    if (changed) {
        Config_Update();
    }
    lua_pushboolean(L, changed);
    return 1;
}

// trxc.config.override(key, value)
static int M_L_ConfigOverride(lua_State *const L)
{
    const CONFIG_OPTION *const option = M_GetOption(L, 1);
    const char *const new_value = M_ValueAsString(L, 2);
    if (!ConfigOverride_PushFromString(option, new_value)) {
        return luaL_error(
            L, "failed to override option %s with %s", option->name, new_value);
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

static const luaL_Reg m_Module[] = {
    { "get", M_L_ConfigGet },
    { "set", M_L_ConfigSet },
    { "reset", M_L_ConfigReset },
    { "override", M_L_ConfigOverride },
    { "restore", M_L_ConfigRestore },
    { "is_overridden", M_L_ConfigIsOverridden },
    { "list", M_L_ConfigList },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "config", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
