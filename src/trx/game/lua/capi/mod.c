#include <trx/game/game_flow.h>
#include <trx/game/game_flow/types.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/field.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/struct.h>
#include <trx/game/lua/utils.h>
#include <trx/game/shell/common.h>
#include <trx/game/shell/mod.h>

#include <lauxlib.h>

// The mods are scanned once at startup and live for the whole session, so a
// handle to one never goes stale. It is addressed by its place in the list,
// which the ref carries as the handle id.

static bool M_GetCanSwitch(const void *const self, TRX_VALUE *const out)
{
    *out =
        (TRX_VALUE) { .type = TVT_BOOL, .as_bool = Shell_CanSwitchToMod(self) };
    return true;
}

// clang-format off
static const FIELD_DESC m_Fields[] = {
    FIELD_RO(SHELL_MOD, name),
    FIELD_RO(SHELL_MOD, title),
    FIELD_RO(SHELL_MOD, mod_type),
    FIELD_RO(SHELL_MOD, engine_version),
    FIELD_RO(SHELL_MOD, base_mod),
    FIELD_RO(SHELL_MOD, is_available),
    FIELD_RO(SHELL_MOD, is_valid),
    FIELD_FN("can_switch", TVT_BOOL, M_GetCanSwitch, nullptr),
};
// clang-format on

TYPE_DEFINE(SHELL_MOD, m_Fields)

static void *M_Resolve(const LUA_STRUCT_REF *const ref)
{
    return (void *)Shell_GetMod(ref->handle.id);
}

static void M_PushMod(lua_State *const L, const int32_t index)
{
    if (Shell_GetMod(index) == nullptr) {
        lua_pushnil(L);
        return;
    }
    LUA_Struct_Push(
        L, &TYPE_SHELL_MOD, M_Resolve, (TRX_HANDLE) { .id = index });
}

// A mod resolves by its place in the list, so a pointer coming from the shell
// is turned back into that index.
static void M_PushModByPtr(lua_State *const L, const SHELL_MOD *const mod)
{
    if (mod == nullptr) {
        lua_pushnil(L);
        return;
    }
    for (int32_t i = 0; i < Shell_GetModCount(); i++) {
        if (Shell_GetMod(i) == mod) {
            M_PushMod(L, i);
            return;
        }
    }
    lua_pushnil(L);
}

// trxc.mod.count() -> int
static int M_L_ModCount(lua_State *const L)
{
    lua_pushinteger(L, Shell_GetModCount());
    return 1;
}

// trxc.mod.get(index) -> SHELL_MOD handle or nil
static int M_L_ModGet(lua_State *const L)
{
    int32_t index;
    if (!LUA_CheckBoundedInt(L, 1, 0, INT32_MAX, &index)) {
        lua_pushnil(L);
        return 1;
    }
    M_PushMod(L, index);
    return 1;
}

// trxc.mod.get_current() -> SHELL_MOD handle or nil
static int M_L_ModGetCurrent(lua_State *const L)
{
    M_PushModByPtr(L, Shell_GetArgs()->startup.mod);
    return 1;
}

// trxc.mod.switch(mod|name) -> bool
static int M_L_ModSwitch(lua_State *const L)
{
    const SHELL_MOD *mod;
    if (lua_type(L, 1) == LUA_TSTRING) {
        mod = Shell_GetModByName(lua_tostring(L, 1));
    } else {
        LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, &TYPE_SHELL_MOD);
        mod = LUA_Struct_Deref(L, ref);
    }

    if (!Shell_CanSwitchToMod(mod)) {
        lua_pushboolean(L, false);
        return 1;
    }

    Shell_RequestModSwitch(mod->name);
    GF_OverrideCommand((GF_COMMAND) { .action = GF_SWITCH_MOD });
    lua_pushboolean(L, true);
    return 1;
}

static const luaL_Reg m_Module[] = {
    { "count", M_L_ModCount },
    { "get", M_L_ModGet },
    { "get_current", M_L_ModGetCurrent },
    { "switch", M_L_ModSwitch },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_Struct_Register(L, &TYPE_SHELL_MOD, nullptr);
    LUA_RegisterModule(L, "mod", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
