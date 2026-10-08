#include <trx/game/lua/registry.h>
#include <trx/game/lua/struct.h>
#include <trx/game/lua/utils/module.h>

static const TYPE_DESC *M_CheckType(lua_State *const L, const int idx)
{
    const char *const name = luaL_checkstring(L, idx);
    const TYPE_DESC *const type = Type_GetByName(name);
    if (type == nullptr) {
        luaL_error(L, "unknown struct type '%s'", name);
    }
    return type;
}

// trxc.struct.members(type: string): { name: string, type: string, writable: boolean }[]
//
// Every member C can reach. Used by the Lua layer to validate a declaration and
// to report members nobody exposed.
static int M_L_StructMembers(lua_State *const L)
{
    const TYPE_DESC *const type = M_CheckType(L, 1);
    lua_newtable(L);
    for (int32_t i = 0; i < type->field_count; i++) {
        const FIELD_DESC *const field = &type->fields[i];
        lua_newtable(L);
        lua_pushstring(L, field->name);
        lua_setfield(L, -2, "name");
        lua_pushstring(L, Value_TypeName(field->type));
        lua_setfield(L, -2, "type");
        lua_pushboolean(L, !(field->flags & FF_READONLY));
        lua_setfield(L, -2, "writable");
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

// trxc.struct.expose_field(type: string, public_name: string, c_name: string, writable?: boolean)
static int M_L_StructExposeField(lua_State *const L)
{
    const TYPE_DESC *const type = M_CheckType(L, 1);
    const char *const public_name = luaL_checkstring(L, 2);
    const char *const c_name = luaL_checkstring(L, 3);
    const bool writable = lua_toboolean(L, 4);

    const FIELD_DESC *const field = Field_Find(type, c_name);
    if (field == nullptr) {
        return luaL_error(
            L, "%s has no member '%s' (declared as '%s')", type->name, c_name,
            public_name);
    }
    // FF_READONLY is the hard floor; a declaration may only narrow it.
    if (writable && (field->flags & FF_READONLY)) {
        return luaL_error(
            L, "%s.%s is read-only in C and cannot be declared writable",
            type->name, c_name);
    }
    LUA_Struct_ExposeField(L, type, public_name, field, writable);
    return 0;
}

// trxc.struct.method(type: string, c_name: string): function
//
// What strict mode wraps. The wrapper goes back through expose_method.
static int M_L_StructMethod(lua_State *const L)
{
    const TYPE_DESC *const type = M_CheckType(L, 1);
    const char *const c_name = luaL_checkstring(L, 2);
    if (!LUA_Struct_PushRawMethod(L, type, c_name)) {
        return luaL_error(L, "%s has no method '%s'", type->name, c_name);
    }
    return 1;
}

// trxc.struct.expose_method(type: string, public_name: string, impl: string|function)
//
// A string names one of the C methods the type offers. A function is exposed as
// it stands, which is how strict mode puts a checking wrapper in front of one.
static int M_L_StructExposeMethod(lua_State *const L)
{
    const TYPE_DESC *const type = M_CheckType(L, 1);
    const char *const public_name = luaL_checkstring(L, 2);

    if (lua_isfunction(L, 3)) {
        lua_pushvalue(L, 3);
    } else {
        const char *const c_name = luaL_checkstring(L, 3);
        if (!LUA_Struct_PushRawMethod(L, type, c_name)) {
            return luaL_error(
                L, "%s has no method '%s' (declared as '%s')", type->name,
                c_name, public_name);
        }
    }
    LUA_Struct_ExposeMethod(L, type, public_name, -1);
    lua_pop(L, 1);
    return 0;
}

// trxc.struct.expose_computed(type: string, public_name: string, fn: function)
static int M_L_StructExposeComputed(lua_State *const L)
{
    const TYPE_DESC *const type = M_CheckType(L, 1);
    const char *const public_name = luaL_checkstring(L, 2);
    luaL_checktype(L, 3, LUA_TFUNCTION);
    LUA_Struct_ExposeComputed(L, type, public_name, 3);
    return 0;
}

static const luaL_Reg m_Module[] = {
    { "members", M_L_StructMembers },
    { "expose_field", M_L_StructExposeField },
    { "method", M_L_StructMethod },
    { "expose_method", M_L_StructExposeMethod },
    { "expose_computed", M_L_StructExposeComputed },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "struct", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
