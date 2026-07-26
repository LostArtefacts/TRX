#include <trx/core/value.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>
#include <trx/game/rules.h>

#include <lauxlib.h>

static const RULE *M_CheckRule(lua_State *const L, const int idx)
{
    const char *const name = luaL_checkstring(L, idx);
    const RULE *const rule = Rules_GetByName(name);
    if (rule == nullptr) {
        luaL_error(L, "unknown rule: %s", name);
    }
    return rule;
}

static TRX_VALUE M_Read(const RULE *const rule)
{
    TRX_VALUE value = {};
    Value_ReadPtr(rule->type, rule->target, &value);
    return value;
}

// trxc.rules.list() -> { "group.field", ... }
static int M_L_RulesList(lua_State *const L)
{
    lua_newtable(L);
    int32_t n = 0;
    for (const RULE *rule = Rules_GetMap(); rule->name != nullptr; rule++) {
        lua_pushstring(L, rule->name);
        lua_rawseti(L, -2, ++n);
    }
    return 1;
}

// trxc.rules.get(name) -> value
static int M_L_RulesGet(lua_State *const L)
{
    const TRX_VALUE value = M_Read(M_CheckRule(L, 1));
    LUA_PushValue(L, &value);
    return 1;
}

// trxc.rules.set(name, value). A string is read as text, as the console gives
// it; anything else is taken as the rule's own type.
static int M_L_RulesSet(lua_State *const L)
{
    const RULE *const rule = M_CheckRule(L, 1);

    TRX_VALUE value = {};
    if (lua_type(L, 2) == LUA_TSTRING) {
        if (!Value_Parse(rule->type, nullptr, lua_tostring(L, 2), &value)) {
            return luaL_error(
                L, "invalid value for %s: %s", rule->name, lua_tostring(L, 2));
        }
    } else {
        value = LUA_CheckValue(L, 2, rule->type);
    }

    const char *const err = Value_WritePtr(rule->type, rule->target, &value);
    if (err != nullptr) {
        return luaL_error(L, "%s", err);
    }
    return 0;
}

// trxc.rules.reset(name). Every rule when the name is omitted.
static int M_L_RulesReset(lua_State *const L)
{
    if (lua_isnoneornil(L, 1)) {
        Rules_Reset();
    } else {
        Rules_ResetOne(M_CheckRule(L, 1));
    }
    return 0;
}

// trxc.rules.format_value(name) -> string
static int M_L_RulesFormatValue(lua_State *const L)
{
    const RULE *const rule = M_CheckRule(L, 1);
    const TRX_VALUE value = M_Read(rule);
    lua_pushstring(L, Value_Format(rule->type, nullptr, &value, true));
    return 1;
}

static const luaL_Reg m_Module[] = {
    { "list", M_L_RulesList },
    { "get", M_L_RulesGet },
    { "set", M_L_RulesSet },
    { "reset", M_L_RulesReset },
    { "format_value", M_L_RulesFormatValue },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "rules", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
