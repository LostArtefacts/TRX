#include <trx/core/enum_map.h>
#include <trx/core/vector.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>

#include <ctype.h>
#include <lauxlib.h>
#include <lualib.h>
#include <stdint.h>
#include <string.h>

// Generic Lua bridge over ENUM_MAP, as struct.c is over FIELD_DESC.
//
// An enum's names and values are written once, in C (see trx/game/enum.c). This
// reflects them into Lua so the declaration there (trx.api.enum) can say which
// constants are public and what they mean without repeating a single number.
//
// The public key is the uppercased ENUM_MAP string, with hyphens written as
// underscores: "active" -> ACTIVE, "top-left" -> TOP_LEFT.

static void M_PushKey(lua_State *const L, const char *const name)
{
    const size_t len = strlen(name);
    luaL_Buffer buffer;
    char *const key = luaL_buffinitsize(L, &buffer, len);
    for (size_t i = 0; i < len; i++) {
        key[i] = name[i] == '-' ? '_' : (char)toupper((unsigned char)name[i]);
    }
    luaL_pushresultsize(&buffer, len);
}

// trxc.enum.values(type) -> { { name = "ACTIVE", value = 1 }, ... }
static int M_L_EnumValues(lua_State *const L)
{
    const char *const type_name = luaL_checkstring(L, 1);
    VECTOR *const values = EnumMap_ListValues(type_name);

    // An enum with no values is a misspelled type name, not an empty enum: a
    // silent {} here would document the enum as having no constants at all.
    // EnumMap_ListValues hands back an empty vector for a name it does not
    // know, and raising is a longjmp, so it has to go before the raise.
    if (values == nullptr || values->count == 0) {
        if (values != nullptr) {
            Vector_Free(values);
        }
        return luaL_argerror(L, 1, "unknown enum");
    }

    lua_newtable(L);
    for (int32_t i = 0; i < values->count; i++) {
        const char *const str_value = *(const char **)Vector_Get(values, i);

        lua_newtable(L);
        M_PushKey(L, str_value);
        lua_setfield(L, -2, "name");
        lua_pushinteger(L, EnumMap_Get(type_name, str_value, 0));
        lua_setfield(L, -2, "value");
        lua_rawseti(L, -2, i + 1);
    }

    Vector_Free(values);
    return 1;
}

static const luaL_Reg m_Module[] = {
    { "values", M_L_EnumValues },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "enum", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
