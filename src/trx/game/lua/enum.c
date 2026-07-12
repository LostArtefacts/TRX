#include <trx/core/enum_map.h>
#include <trx/core/memory.h>
#include <trx/core/vector.h>

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
// The public key is the uppercased ENUM_MAP string: "active" -> ACTIVE.

// trxc.enum.values(type) -> { { name = "ACTIVE", value = 1 }, ... }
static int M_L_Values(lua_State *const L)
{
    const char *const type_name = luaL_checkstring(L, 1);
    VECTOR *const values = EnumMap_ListValues(type_name);
    // An enum with no values is a misspelled type name, not an empty enum: a
    // silent {} here would document the enum as having no constants at all.
    luaL_argcheck(L, values != nullptr && values->count > 0, 1, "unknown enum");

    lua_newtable(L);
    for (int32_t i = 0; i < values->count; i++) {
        const char *const str_value = *(const char **)Vector_Get(values, i);

        const size_t len = strlen(str_value);
        char *const key = Memory_Alloc(len + 1);
        for (size_t j = 0; j < len; j++) {
            key[j] = (char)toupper((unsigned char)str_value[j]);
        }
        key[len] = '\0';

        lua_newtable(L);
        lua_pushstring(L, key);
        lua_setfield(L, -2, "name");
        lua_pushinteger(L, EnumMap_Get(type_name, str_value, 0));
        lua_setfield(L, -2, "value");
        lua_rawseti(L, -2, i + 1);

        Memory_Free(key);
    }

    Vector_Free(values);
    return 1;
}

void LUA_CreateEnum(lua_State *const L)
{
    lua_getglobal(L, "trxc");
    lua_newtable(L);
    lua_pushcfunction(L, M_L_Values);
    lua_setfield(L, -2, "values");
    lua_setfield(L, -2, "enum");
    lua_pop(L, 1);
}
