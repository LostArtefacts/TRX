#include <trx/game/lua/struct.h>

#include <trx/game/lua/utils.h>

#include <string.h>

// Metatable keys. __fields / __methods / __ext are the public surface, and are
// empty until a script declares it. __raw_methods is every method C could
// offer; it is never reachable from a handle - Lua selects from it by name.
static const char m_KeyFields[] = "__fields";
static const char m_KeyWritable[] = "__writable";
static const char m_KeyMethods[] = "__methods";
static const char m_KeyExt[] = "__ext";
static const char m_KeyRawMethods[] = "__raw_methods";

// Upvalue 1 of __index and __newindex maps public name -> FIELD_DESC *. A
// linear strcmp scan over the field table measured ~43ns per access; this is a
// single interned-string hash lookup.
static const FIELD_DESC *M_LookUpField(lua_State *const L, const int key_idx)
{
    lua_pushvalue(L, key_idx);
    lua_rawget(L, lua_upvalueindex(1));
    const FIELD_DESC *const field = lua_touserdata(L, -1);
    lua_pop(L, 1);
    return field;
}

static int M_Index(lua_State *const L)
{
    LUA_STRUCT_REF *const ref = lua_touserdata(L, 1);

    const FIELD_DESC *const field = M_LookUpField(L, 2);
    if (field != nullptr) {
        TRX_VALUE value;
        if (!Field_Get(field, LUA_Struct_Deref(L, ref), &value)) {
            lua_pushnil(L);
            return 1;
        }
        LUA_PushMemberValue(L, &value, 1, 2);
        return 1;
    }

    // Upvalue 2: methods. Returned as-is; Lua calls them with the userdata as
    // self, so no rebinding wrapper is needed.
    lua_pushvalue(L, 2);
    if (lua_rawget(L, lua_upvalueindex(2)) != LUA_TNIL) {
        return 1;
    }
    lua_pop(L, 1);

    // Upvalue 3: computed members declared in Lua (item.room, item.properties).
    // Invoked now, with the userdata; the result is the value.
    lua_pushvalue(L, 2);
    if (lua_rawget(L, lua_upvalueindex(3)) != LUA_TFUNCTION) {
        lua_pop(L, 1);
        lua_pushnil(L);
        return 1;
    }
    lua_pushvalue(L, 1);
    lua_call(L, 1, 1);
    return 1;
}

static int M_NewIndex(lua_State *const L)
{
    LUA_STRUCT_REF *const ref = lua_touserdata(L, 1);

    // Upvalue 1 is the *writable* subset, not every field. A field the
    // declaration marked read-only is absent here even when the C member is
    // plain and Field_Set would happily write it: writability is the
    // declaration's to decide, and several members are read-only precisely
    // because writing them directly would wedge engine state.
    const FIELD_DESC *const field = M_LookUpField(L, 2);
    if (field == nullptr) {
        // Upvalue 2 is every declared field, so a read-only one can be named as
        // such instead of being reported as unknown.
        lua_pushvalue(L, 2);
        lua_rawget(L, lua_upvalueindex(2));
        const bool declared = !lua_isnil(L, -1);
        lua_pop(L, 1);
        if (declared) {
            return luaL_error(
                L, "%s.%s is read-only", ref->type->name, lua_tostring(L, 2));
        }
        return luaL_error(
            L, "unknown %s field '%s'", ref->type->name, lua_tostring(L, 2));
    }

    const bool is_null =
        (field->flags & FF_NULLABLE) != 0 && lua_isnoneornil(L, 3);
    TRX_VALUE value;
    if (!is_null) {
        value = LUA_CheckValue(L, 3, field->type);
    }
    const char *const err =
        Field_Set(field, LUA_Struct_Deref(L, ref), is_null ? nullptr : &value);
    if (err != nullptr) {
        return luaL_error(
            L, "cannot set %s.%s: %s", ref->type->name, lua_tostring(L, 2),
            err);
    }
    return 0;
}

// Unlike the metamethods, this one is a value in its own right: __pairs hands
// it to the script, which is then free to call it with anything at all. Upvalue
// 2 carries the type so that anything else is refused rather than dereferenced.
static int M_PairsIter(lua_State *const L)
{
    const TYPE_DESC *const type = lua_touserdata(L, lua_upvalueindex(2));
    LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, type);
    void *const self = LUA_Struct_Deref(L, ref);

    // Iterate the declared public fields, not the C table: a member C can reach
    // but no script declared is not part of the type.
    lua_pushvalue(L, 2);
    while (lua_next(L, lua_upvalueindex(1)) != 0) {
        const FIELD_DESC *const field = lua_touserdata(L, -1);
        lua_pop(L, 1); // value; the public name stays on the stack as the key

        TRX_VALUE value;
        if (Field_Get(field, self, &value)) {
            lua_pushvalue(
                L, -1); // key again, as the iterator's control variable
            lua_insert(L, -2);
            LUA_PushValue(L, &value);
            return 2;
        }
        // A getter that declines has nothing to yield. Yielding nil would end
        // the iteration, hiding every field behind it.
    }

    lua_pushnil(L);
    return 1;
}

// LUA_Struct_Push mints a fresh userdata every time, so two handles to the same
// thing are never the same value. Compare what they point at instead: the type,
// and the handle that names the entity and its occupant.
static int M_Eq(lua_State *const L)
{
    // Lua takes __eq from the first operand, or from the second when the first
    // has none, so neither side is known to be a handle of this type. Upvalue 1
    // carries the type; anything else is unequal rather than dereferenced.
    const TYPE_DESC *const type = lua_touserdata(L, lua_upvalueindex(1));
    const LUA_STRUCT_REF *const a = luaL_testudata(L, 1, type->name);
    const LUA_STRUCT_REF *const b = luaL_testudata(L, 2, type->name);
    lua_pushboolean(
        L, a != nullptr && b != nullptr && Handle_Equal(a->handle, b->handle));
    return 1;
}

// __index, __newindex and __pairs are only ever reached through a handle's
// metatable, and that metatable is protected, so arg 1 is always a handle of
// this type. The iterator __pairs returns is the one thing here that escapes.
static int M_Pairs(lua_State *const L)
{
    const TYPE_DESC *const type =
        ((LUA_STRUCT_REF *)lua_touserdata(L, 1))->type;
    luaL_getmetatable(L, type->name);
    lua_getfield(L, -1, m_KeyFields);
    lua_remove(L, -2);
    lua_pushlightuserdata(L, (void *)type);
    lua_pushcclosure(L, M_PairsIter, 2);
    lua_pushvalue(L, 1);
    lua_pushnil(L);
    return 3;
}

// Whether a handle still resolves. Every type offers it; a declaration exposes
// it by name. Upvalue 1 carries the type, and it typechecks its argument as
// M_Eq does.
static int M_IsValid(lua_State *const L)
{
    const TYPE_DESC *const type = lua_touserdata(L, lua_upvalueindex(1));
    LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, type);
    lua_pushboolean(L, ref->resolve(ref) != nullptr);
    return 1;
}

// Each bridge closes over its LUA_PROPERTY_DESC in upvalue 1.
static int M_PropertyGet(lua_State *const L)
{
    const LUA_PROPERTY_DESC *const desc =
        lua_touserdata(L, lua_upvalueindex(1));
    LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, desc->type);
    const void *const self = LUA_Struct_Deref(L, ref);
    TRX_VALUE value = {};
    if (!desc->get(self, luaL_checkstring(L, 2), &value)) {
        lua_pushnil(L);
        return 1;
    }
    LUA_PushValue(L, &value);
    return 1;
}

static int M_PropertySet(lua_State *const L)
{
    const LUA_PROPERTY_DESC *const desc =
        lua_touserdata(L, lua_upvalueindex(1));
    LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, desc->type);
    void *const self = LUA_Struct_Deref(L, ref);
    const char *const name = luaL_checkstring(L, 2);
    // As in M_NewIndex, a name nobody declared is reported as such rather than
    // as a value the property would not take.
    TRX_VALUE existing = {};
    if (!desc->get(self, name, &existing)) {
        return luaL_error(L, "unknown %s property '%s'", desc->what, name);
    }
    const TRX_VALUE value = LUA_CheckValue(L, 3, existing.type);
    const char *const err = desc->set(self, name, value);
    if (err != nullptr) {
        return luaL_error(
            L, "cannot set %s property '%s': %s", desc->what, name, err);
    }
    return 0;
}

static int M_PropertyGetNames(lua_State *const L)
{
    const LUA_PROPERTY_DESC *const desc =
        lua_touserdata(L, lua_upvalueindex(1));
    LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, desc->type);
    const void *const self = LUA_Struct_Deref(L, ref);
    const int32_t count = desc->name_count(self);
    lua_createtable(L, count, 0);
    for (int32_t i = 0; i < count; i++) {
        lua_pushstring(L, desc->name_at(self, i));
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

// Puts the value at val_idx under public_name into the type's metatable
// subtable named `key`. Leaves the stack as it found it.
static void M_ExposeInto(
    lua_State *const L, const TYPE_DESC *const type, const char *const key,
    const char *const public_name, const int val_idx)
{
    const int val = lua_absindex(L, val_idx);
    luaL_getmetatable(L, type->name);
    lua_getfield(L, -1, key);
    lua_pushstring(L, public_name);
    lua_pushvalue(L, val);
    lua_rawset(L, -3);
    lua_pop(L, 2); // subtable, metatable
}

LUA_STRUCT_REF *LUA_Struct_CheckRef(
    lua_State *const L, const int idx, const TYPE_DESC *const type)
{
    return luaL_checkudata(L, idx, type->name);
}

void *LUA_Struct_Deref(lua_State *const L, LUA_STRUCT_REF *const ref)
{
    void *const self = ref->resolve(ref);
    if (self == nullptr) {
        luaL_error(L, "stale %s handle", ref->type->name);
    }
    return self;
}

void LUA_Struct_Register(
    lua_State *const L, const TYPE_DESC *const type,
    const luaL_Reg *const methods)
{
    Field_ValidateType(type);

    luaL_newmetatable(L, type->name);

    // Everything C could offer, for Lua to select from. Not reachable from a
    // handle.
    lua_newtable(L);
    if (methods != nullptr) {
        luaL_setfuncs(L, methods, 0);
    }
    lua_pushlightuserdata(L, (void *)type);
    lua_pushcclosure(L, M_IsValid, 1);
    lua_setfield(L, -2, "is_valid");
    lua_setfield(L, -2, m_KeyRawMethods);

    // The public surface: empty until a script declares it. `writable` is the
    // subset of `fields` the declaration allows a script to set.
    lua_newtable(L); // fields
    lua_newtable(L); // writable
    lua_newtable(L); // methods
    lua_newtable(L); // ext

    lua_pushvalue(L, -4);
    lua_setfield(L, -6, m_KeyFields);
    lua_pushvalue(L, -3);
    lua_setfield(L, -6, m_KeyWritable);
    lua_pushvalue(L, -2);
    lua_setfield(L, -6, m_KeyMethods);
    lua_pushvalue(L, -1);
    lua_setfield(L, -6, m_KeyExt);

    // __index and __newindex close over these tables. Populating them later
    // from Lua is visible here, because the upvalues are the tables themselves.
    lua_pushvalue(L, -4); // fields
    lua_pushvalue(L, -3); // methods
    lua_pushvalue(L, -3); // ext
    lua_pushcclosure(L, M_Index, 3);
    lua_setfield(L, -6, "__index");

    lua_pushvalue(L, -3); // writable
    lua_pushvalue(L, -5); // fields
    lua_pushcclosure(L, M_NewIndex, 2);
    lua_setfield(L, -6, "__newindex");

    lua_pop(L, 4); // fields, writable, methods, ext

    lua_pushcfunction(L, M_Pairs);
    lua_setfield(L, -2, "__pairs");

    lua_pushlightuserdata(L, (void *)type);
    lua_pushcclosure(L, M_Eq, 1);
    lua_setfield(L, -2, "__eq");

    // Protect the metatable. Without this, getmetatable(item).__raw_methods
    // hands a script every C method the type could offer, including the ones no
    // declaration exposed - which would make the opt-in surface a fiction.
    lua_pushstring(L, type->name);
    lua_setfield(L, -2, "__metatable");

    lua_pop(L, 1);
}

void LUA_Struct_Push(
    lua_State *const L, const TYPE_DESC *const type,
    void *(*const resolve)(const LUA_STRUCT_REF *), const TRX_HANDLE handle)
{
    LUA_STRUCT_REF *const ref = lua_newuserdatauv(L, sizeof(LUA_STRUCT_REF), 0);
    *ref = (LUA_STRUCT_REF) {
        .type = type,
        .resolve = resolve,
        .handle = handle,
    };
    luaL_setmetatable(L, type->name);
}

void LUA_Property_Register(
    lua_State *const L, const LUA_PROPERTY_DESC *const desc)
{
    static const struct {
        const char *name;
        lua_CFunction fn;
    } bridges[] = {
        { "get_property", M_PropertyGet },
        { "set_property", M_PropertySet },
        { "get_property_names", M_PropertyGetNames },
    };

    luaL_getmetatable(L, desc->type->name);
    lua_getfield(L, -1, m_KeyRawMethods);
    for (size_t i = 0; i < sizeof(bridges) / sizeof(bridges[0]); i++) {
        lua_pushlightuserdata(L, (void *)desc);
        lua_pushcclosure(L, bridges[i].fn, 1);
        lua_setfield(L, -2, bridges[i].name);
    }
    lua_pop(L, 2);
}

void LUA_Struct_ExposeField(
    lua_State *const L, const TYPE_DESC *const type,
    const char *const public_name, const FIELD_DESC *const field,
    const bool writable)
{
    lua_pushlightuserdata(L, (void *)field);
    M_ExposeInto(L, type, m_KeyFields, public_name, -1);
    // Read-only is not merely documentation: a field the declaration withholds
    // must be absent from the writable set, or __newindex would let a script
    // set it anyway.
    if (writable) {
        M_ExposeInto(L, type, m_KeyWritable, public_name, -1);
    }
    lua_pop(L, 1);
}

void LUA_Struct_ExposeMethod(
    lua_State *const L, const TYPE_DESC *const type,
    const char *const public_name, const int fn_idx)
{
    M_ExposeInto(L, type, m_KeyMethods, public_name, fn_idx);
}

void LUA_Struct_ExposeComputed(
    lua_State *const L, const TYPE_DESC *const type,
    const char *const public_name, const int fn_idx)
{
    M_ExposeInto(L, type, m_KeyExt, public_name, fn_idx);
}

bool LUA_Struct_PushRawMethod(
    lua_State *const L, const TYPE_DESC *const type, const char *const c_name)
{
    luaL_getmetatable(L, type->name);
    lua_getfield(L, -1, m_KeyRawMethods);
    const bool found = lua_getfield(L, -1, c_name) == LUA_TFUNCTION;
    lua_remove(L, -2);
    lua_remove(L, -2);
    return found;
}
