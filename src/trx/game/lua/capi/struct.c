#include <trx/game/lua/registry.h>
#include <trx/game/lua/struct.h>
#include <trx/game/lua/utils.h>

#include <string.h>

// Metatable keys. __fields / __methods / __ext are the public surface, and are
// empty until a script declares it. __raw_methods is every method C could
// offer; it is never reachable from a handle - Lua selects from it by name.
static const char M_KEY_FIELDS[] = "__fields";
static const char M_KEY_WRITABLE[] = "__writable";
static const char M_KEY_METHODS[] = "__methods";
static const char M_KEY_EXT[] = "__ext";
static const char M_KEY_RAW_METHODS[] = "__raw_methods";

static void M_PushValue(lua_State *const L, const FIELD_VALUE *const value)
{
    switch (value->type) {
    case FT_BOOL:
        lua_pushboolean(L, value->as_bool);
        break;
    case FT_FLOAT:
    case FT_DOUBLE:
        lua_pushnumber(L, value->as_num);
        break;
    case FT_STRING:
        if (value->as_str == nullptr) {
            lua_pushnil(L);
        } else {
            lua_pushstring(L, value->as_str);
        }
        break;
    case FT_XYZ_16:
    case FT_XYZ_32:
        LUA_PushXYZ(L, value->as_xyz);
        break;
    default:
        lua_pushinteger(L, value->as_int);
        break;
    }
}

static FIELD_VALUE M_CheckValue(
    lua_State *const L, const int idx, const FIELD_TYPE type)
{
    FIELD_VALUE value = { .type = type };
    switch (type) {
    case FT_BOOL:
        luaL_checktype(L, idx, LUA_TBOOLEAN);
        value.as_bool = lua_toboolean(L, idx);
        break;
    case FT_FLOAT:
    case FT_DOUBLE:
        value.as_num = luaL_checknumber(L, idx);
        break;
    case FT_STRING:
        // nil clears a string field; its setter decides whether that is
        // allowed (e.g. item.name = nil removes the name). A field with no
        // custom setter still rejects the write in Field_Set.
        value.as_str =
            lua_isnoneornil(L, idx) ? nullptr : luaL_checkstring(L, idx);
        break;
    case FT_XYZ_16:
    case FT_XYZ_32:
        value.as_xyz = LUA_CheckXYZ(L, idx);
        break;
    default:
        value.as_int = luaL_checkinteger(L, idx);
        break;
    }
    return value;
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
        FIELD_VALUE value;
        if (!Field_Get(field, LUA_Struct_Deref(L, ref), &value)) {
            lua_pushnil(L);
            return 1;
        }
        M_PushValue(L, &value);
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

    const FIELD_VALUE value = M_CheckValue(L, 3, field->type);
    const char *const err = Field_Set(field, LUA_Struct_Deref(L, ref), &value);
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

        FIELD_VALUE value;
        if (Field_Get(field, self, &value)) {
            lua_pushvalue(
                L, -1); // key again, as the iterator's control variable
            lua_insert(L, -2);
            M_PushValue(L, &value);
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
// the slot, and the generation that says which occupant of the slot is meant.
static int M_Eq(lua_State *const L)
{
    // Lua takes __eq from the first operand, or from the second when the first
    // has none, so neither side is known to be a handle of this type. Upvalue 1
    // carries the type; anything else is unequal rather than dereferenced.
    const TYPE_DESC *const type = lua_touserdata(L, lua_upvalueindex(1));
    const LUA_STRUCT_REF *const a = luaL_testudata(L, 1, type->name);
    const LUA_STRUCT_REF *const b = luaL_testudata(L, 2, type->name);
    lua_pushboolean(
        L,
        a != nullptr && b != nullptr && a->idx == b->idx && a->gen == b->gen);
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
    lua_getfield(L, -1, M_KEY_FIELDS);
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
    lua_setfield(L, -2, M_KEY_RAW_METHODS);

    // The public surface: empty until a script declares it. `writable` is the
    // subset of `fields` the declaration allows a script to set.
    lua_newtable(L); // fields
    lua_newtable(L); // writable
    lua_newtable(L); // methods
    lua_newtable(L); // ext

    lua_pushvalue(L, -4);
    lua_setfield(L, -6, M_KEY_FIELDS);
    lua_pushvalue(L, -3);
    lua_setfield(L, -6, M_KEY_WRITABLE);
    lua_pushvalue(L, -2);
    lua_setfield(L, -6, M_KEY_METHODS);
    lua_pushvalue(L, -1);
    lua_setfield(L, -6, M_KEY_EXT);

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
    void *(*const resolve)(const LUA_STRUCT_REF *), const int32_t idx,
    const uint32_t gen)
{
    LUA_STRUCT_REF *const ref = lua_newuserdatauv(L, sizeof(LUA_STRUCT_REF), 0);
    *ref = (LUA_STRUCT_REF) {
        .type = type,
        .resolve = resolve,
        .idx = idx,
        .gen = gen,
    };
    luaL_setmetatable(L, type->name);
}

// A property carrier tells float and double apart; a field carrier widens both
// into as_num. What each becomes in Lua is the same either way, so a property
// is pushed as the field value it stands for rather than through a second
// switch that has to agree with M_PushValue.
static void M_PushPropertyValue(
    lua_State *const L, const OBJECT_PROPERTY_VALUE *const value)
{
    FIELD_VALUE as_field = {};
    switch (value->type) {
    case OBJECT_PROPERTY_TYPE_INT:
        as_field = (FIELD_VALUE) { .type = FT_INT32, .as_int = value->as_int };
        break;
    case OBJECT_PROPERTY_TYPE_FLOAT:
        as_field =
            (FIELD_VALUE) { .type = FT_FLOAT, .as_num = value->as_float };
        break;
    case OBJECT_PROPERTY_TYPE_DOUBLE:
        as_field =
            (FIELD_VALUE) { .type = FT_DOUBLE, .as_num = value->as_double };
        break;
    case OBJECT_PROPERTY_TYPE_BOOL:
        as_field = (FIELD_VALUE) { .type = FT_BOOL, .as_bool = value->as_bool };
        break;
    case OBJECT_PROPERTY_TYPE_XYZ:
        as_field = (FIELD_VALUE) { .type = FT_XYZ_32, .as_xyz = value->as_xyz };
        break;
    }
    M_PushValue(L, &as_field);
}

static OBJECT_PROPERTY_VALUE M_CheckPropertyValue(
    lua_State *const L, const int arg)
{
    switch (lua_type(L, arg)) {
    case LUA_TBOOLEAN:
        return (OBJECT_PROPERTY_VALUE) {
            .type = OBJECT_PROPERTY_TYPE_BOOL,
            .as_bool = lua_toboolean(L, arg),
        };

    case LUA_TNUMBER:
        if (lua_isinteger(L, arg)) {
            return (OBJECT_PROPERTY_VALUE) {
                .type = OBJECT_PROPERTY_TYPE_INT,
                .as_int = lua_tointeger(L, arg),
            };
        }
        return (OBJECT_PROPERTY_VALUE) {
            .type = OBJECT_PROPERTY_TYPE_DOUBLE,
            .as_double = lua_tonumber(L, arg),
        };

    case LUA_TTABLE:
        return (OBJECT_PROPERTY_VALUE) {
            .type = OBJECT_PROPERTY_TYPE_XYZ,
            .as_xyz = LUA_CheckXYZ(L, arg),
        };

    default:
        break;
    }

    luaL_error(L, "property value must be a number, boolean or table");
    return (OBJECT_PROPERTY_VALUE) {};
}

// Each bridge closes over its LUA_PROPERTY_DESC in upvalue 1.
static int M_PropertyGet(lua_State *const L)
{
    const LUA_PROPERTY_DESC *const desc =
        lua_touserdata(L, lua_upvalueindex(1));
    LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, desc->type);
    const void *const self = LUA_Struct_Deref(L, ref);
    OBJECT_PROPERTY_VALUE value = {};
    if (!desc->get(self, luaL_checkstring(L, 2), &value)) {
        lua_pushnil(L);
        return 1;
    }
    M_PushPropertyValue(L, &value);
    return 1;
}

static int M_PropertySet(lua_State *const L)
{
    const LUA_PROPERTY_DESC *const desc =
        lua_touserdata(L, lua_upvalueindex(1));
    LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, desc->type);
    void *const self = LUA_Struct_Deref(L, ref);
    const char *const name = luaL_checkstring(L, 2);
    const OBJECT_PROPERTY_VALUE value = M_CheckPropertyValue(L, 3);
    if (!desc->set(self, name, value)) {
        return luaL_error(L, "unknown %s property '%s'", desc->what, name);
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
    lua_getfield(L, -1, M_KEY_RAW_METHODS);
    for (size_t i = 0; i < sizeof(bridges) / sizeof(bridges[0]); i++) {
        lua_pushlightuserdata(L, (void *)desc);
        lua_pushcclosure(L, bridges[i].fn, 1);
        lua_setfield(L, -2, bridges[i].name);
    }
    lua_pop(L, 2);
}

// --- trxc.struct: how Lua declares the public surface -----------------------

static const TYPE_DESC *M_CheckType(lua_State *const L, const int idx)
{
    const char *const name = luaL_checkstring(L, idx);
    const TYPE_DESC *const type = Type_GetByName(name);
    if (type == nullptr) {
        luaL_error(L, "unknown struct type '%s'", name);
    }
    return type;
}

// trxc.struct.members(type) -> { {name=, type=, writable=}, ... }
//
// Every member C can reach. Used by the Lua layer to validate a declaration and
// to report members nobody exposed.
static int M_L_Members(lua_State *const L)
{
    const TYPE_DESC *const type = M_CheckType(L, 1);
    lua_newtable(L);
    for (int32_t i = 0; i < type->field_count; i++) {
        const FIELD_DESC *const field = &type->fields[i];
        lua_newtable(L);
        lua_pushstring(L, field->name);
        lua_setfield(L, -2, "name");
        lua_pushstring(L, Field_GetTypeName(field->type));
        lua_setfield(L, -2, "type");
        lua_pushboolean(L, !(field->flags & FF_READONLY));
        lua_setfield(L, -2, "writable");
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

// trxc.struct.expose_field(type, public_name, c_name, writable)
static int M_L_ExposeField(lua_State *const L)
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
    if (writable && (field->flags & FF_READONLY)) {
        return luaL_error(
            L, "%s.%s is read-only in C and cannot be declared writable",
            type->name, c_name);
    }

    luaL_getmetatable(L, type->name);

    lua_getfield(L, -1, M_KEY_FIELDS);
    lua_pushstring(L, public_name);
    lua_pushlightuserdata(L, (void *)field);
    lua_rawset(L, -3);
    lua_pop(L, 1);

    // Read-only is not merely documentation: a field the declaration withholds
    // must be absent from the writable set, or __newindex would let a script
    // set it anyway. FF_READONLY is the hard floor, checked above; this is the
    // declaration narrowing a member C would otherwise write.
    if (writable) {
        lua_getfield(L, -1, M_KEY_WRITABLE);
        lua_pushstring(L, public_name);
        lua_pushlightuserdata(L, (void *)field);
        lua_rawset(L, -3);
        lua_pop(L, 1);
    }
    return 0;
}

// trxc.struct.expose_method(type, public_name, c_name)
static int M_L_ExposeMethod(lua_State *const L)
{
    const TYPE_DESC *const type = M_CheckType(L, 1);
    const char *const public_name = luaL_checkstring(L, 2);
    const char *const c_name = luaL_checkstring(L, 3);

    luaL_getmetatable(L, type->name);
    lua_getfield(L, -1, M_KEY_RAW_METHODS);
    if (lua_getfield(L, -1, c_name) != LUA_TFUNCTION) {
        return luaL_error(
            L, "%s has no method '%s' (declared as '%s')", type->name, c_name,
            public_name);
    }

    lua_getfield(L, -3, M_KEY_METHODS);
    lua_pushstring(L, public_name);
    lua_pushvalue(L, -3); // the C function
    lua_rawset(L, -3);
    return 0;
}

// trxc.struct.expose_computed(type, public_name, fn)
static int M_L_ExposeComputed(lua_State *const L)
{
    const TYPE_DESC *const type = M_CheckType(L, 1);
    const char *const public_name = luaL_checkstring(L, 2);
    luaL_checktype(L, 3, LUA_TFUNCTION);

    luaL_getmetatable(L, type->name);
    lua_getfield(L, -1, M_KEY_EXT);
    lua_pushstring(L, public_name);
    lua_pushvalue(L, 3);
    lua_rawset(L, -3);
    return 0;
}

static const luaL_Reg m_Module[] = {
    { "members", M_L_Members },
    { "expose_field", M_L_ExposeField },
    { "expose_method", M_L_ExposeMethod },
    { "expose_computed", M_L_ExposeComputed },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "struct", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
