#include <trx/config/common.h>
#include <trx/config/option.h>
#include <trx/config/registry.h>
#include <trx/core/dynamic_enum.h>
#include <trx/core/enum_map.h>
#include <trx/core/vector.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>

#include <lauxlib.h>

static CONFIG_OPTION *M_GetOption(lua_State *const L, const int32_t arg)
{
    const char *const key = luaL_checkstring(L, arg);
    CONFIG_OPTION *const option = Config_FindOption(key);
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
    const TRX_VALUE_TYPE type = option->value.type;
    if (type == TVT_RGB_888 || type == TVT_ENUM || type == TVT_STRING
        || type == TVT_DYNAMIC_ENUM) {
        lua_pushstring(L, Config_Option_GetValueAsString(option, false));
        return;
    }
    LUA_PushValue(L, &option->value);
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

// The name a value shape goes by in a description.
static const char *M_KindName(const TRX_VALUE_TYPE type)
{
    switch (type) {
    case TVT_BOOL:
        return "boolean";
    case TVT_S8:
    case TVT_U8:
    case TVT_S16:
    case TVT_U16:
    case TVT_S32:
    case TVT_U32:
        return "integer";
    case TVT_FLOAT:
    case TVT_DOUBLE:
        return "number";
    case TVT_XYZ_16:
    case TVT_XYZ_32:
        return "xyz";
    case TVT_RGB_888:
        return "color";
    case TVT_ENUM:
        return "enum";
    case TVT_DYNAMIC_ENUM:
        return "dynamic_enum";
    case TVT_STRING:
        return "string";
    }
    return "";
}

// trxc.config.describe(key) -> table
static int M_L_ConfigDescribe(lua_State *const L)
{
    const CONFIG_OPTION *const option = M_GetOption(L, 1);
    lua_newtable(L);
    lua_pushstring(L, M_KindName(option->value.type));
    lua_setfield(L, -2, "kind");
    lua_pushboolean(L, (option->flags & CONFIG_OPTION_PERCENT) != 0);
    lua_setfield(L, -2, "percent");

    if (option->value.type == TVT_ENUM) {
        VECTOR *const values = EnumMap_ListValues(option->enum_map);
        lua_createtable(L, values != nullptr ? values->count : 0, 0);
        if (values != nullptr) {
            for (int32_t i = 0; i < values->count; i++) {
                lua_pushstring(L, *(char **)Vector_Get(values, i));
                lua_rawseti(L, -2, i + 1);
            }
            Vector_Free(values);
        }
        lua_setfield(L, -2, "values");
    } else if (option->value.type == TVT_DYNAMIC_ENUM) {
        const void *const token = Config_Option_GetEnumKey(option);
        const int32_t count = DynamicEnum_GetValueCount(token);
        lua_createtable(L, count, 0);
        int32_t n = 0;
        for (int32_t i = 0; i < count; i++) {
            const char *const value = DynamicEnum_GetValueAt(token, i);
            if (value != nullptr) {
                lua_pushstring(L, value);
                n++;
                lua_rawseti(L, -2, n);
            }
        }
        lua_setfield(L, -2, "values");
    }
    return 1;
}

// trxc.config.set(key, value, force?)
static int M_L_ConfigSet(lua_State *const L)
{
    CONFIG_OPTION *const option = M_GetOption(L, 1);
    const char *const new_value = M_ValueAsString(L, 2);
    const bool force = lua_toboolean(L, 3);
    if (!Config_Option_SetFromString(option, new_value, force)) {
        return luaL_error(
            L, "failed to set option %s to %s", option->name, new_value);
    }
    Config_Update();
    return 0;
}

// trxc.config.reset(key, force?) -> bool
static int M_L_ConfigReset(lua_State *const L)
{
    CONFIG_OPTION *const option = M_GetOption(L, 1);
    const bool force = lua_toboolean(L, 2);
    const bool changed = Config_Option_RestoreDefault(option, force);
    if (changed) {
        Config_Update();
    }
    lua_pushboolean(L, changed);
    return 1;
}

// trxc.config.override(key, value)
static int M_L_ConfigOverride(lua_State *const L)
{
    CONFIG_OPTION *const option = M_GetOption(L, 1);
    const char *const new_value = M_ValueAsString(L, 2);
    if (!Config_Option_PushHoldFromString(
            option, new_value, CONFIG_HOLD_SCRIPT)) {
        return luaL_error(
            L, "failed to override option %s with %s", option->name, new_value);
    }
    Config_Update();
    return 0;
}

// trxc.config.restore(key) -> bool
static int M_L_ConfigRestore(lua_State *const L)
{
    CONFIG_OPTION *const option = M_GetOption(L, 1);
    const bool restored = Config_Option_PopHold(option);
    if (restored) {
        Config_Update();
    }
    lua_pushboolean(L, restored);
    return 1;
}

// trxc.config.is_overridden(key) -> bool
static int M_L_ConfigIsOverridden(lua_State *const L)
{
    const CONFIG_OPTION *const option = M_GetOption(L, 1);
    lua_pushboolean(L, Config_Option_IsHeld(option));
    return 1;
}

// trxc.config.list()
static int M_L_ConfigList(lua_State *const L)
{
    lua_newtable(L);
    for (CONFIG_OPTION *const *option = Config_GetOptions(); *option != nullptr;
         option++) {
        M_PushValue(L, *option);
        lua_setfield(L, -2, (*option)->name);
    }
    return 1;
}

static const luaL_Reg m_Module[] = {
    { "get", M_L_ConfigGet },
    { "describe", M_L_ConfigDescribe },
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
