#include <trx/core/strings.h>
#include <trx/core/strings/fuzzy_match.h>
#include <trx/game/lua/common.h>

#include <lauxlib.h>

// Reads a field off the table on top of the stack without going through
// __index, so that what M_CheckSources saw is what the match is built from.
static void M_PushRawField(lua_State *const L, const char *const name)
{
    lua_pushstring(L, name);
    lua_rawget(L, -2);
}

// A source has to hold a non-empty string key. A key read out of any other kind
// of field would be a string that only the stack slot holds, and it is popped
// long before String_FuzzyMatch reads it; an empty one divides by zero in the
// scorer, which measures how much of the key the input covers. Checking before
// the vector exists also keeps an error from leaking it.
static void M_CheckSources(lua_State *const L, const int32_t arg)
{
    const int32_t count = (int32_t)lua_rawlen(L, arg);
    for (int32_t i = 1; i <= count; i++) {
        lua_rawgeti(L, arg, i);
        luaL_argcheck(L, lua_istable(L, -1), arg, "expected a list of tables");

        M_PushRawField(L, "key");
        luaL_argcheck(
            L, lua_type(L, -1) == LUA_TSTRING, arg,
            "every source needs a string key");
        luaL_argcheck(L, lua_rawlen(L, -1) > 0, arg, "a key cannot be empty");
        lua_pop(L, 1);

        M_PushRawField(L, "weight");
        luaL_argcheck(
            L, lua_isnoneornil(L, -1) || lua_isinteger(L, -1), arg,
            "a weight has to be an integer");
        lua_pop(L, 2);
    }
}

// trxc.strings.fuzzy_match(input, sources) -> matches
//
// `sources` is a list of { key, value, weight }. The value is the caller's own:
// it is not read here, and it comes back on the match.
static int M_L_StringsFuzzyMatch(lua_State *const L)
{
    const char *const input = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);
    M_CheckSources(L, 2);

    VECTOR *const sources = Vector_Create(sizeof(STRING_FUZZY_SOURCE));

    const int32_t count = (int32_t)lua_rawlen(L, 2);
    for (int32_t i = 1; i <= count; i++) {
        lua_rawgeti(L, 2, i);

        // The source table holds the key, so the pointer outlives the pop.
        M_PushRawField(L, "key");
        const char *const key = lua_tostring(L, -1);
        lua_pop(L, 1);

        M_PushRawField(L, "weight");
        const int32_t weight = (int32_t)luaL_optinteger(L, -1, 1);
        lua_pop(L, 1);

        // Carry the 1-based index rather than the value itself, so nothing here
        // holds a reference into the caller's table.
        const STRING_FUZZY_SOURCE source = {
            .key = key,
            .value = (void *)(intptr_t)i,
            .weight = weight,
        };
        Vector_Add(sources, (void *)&source);
        lua_pop(L, 1);
    }

    VECTOR *const matches = String_FuzzyMatch(input, sources);

    lua_newtable(L);
    for (int32_t i = 0; i < matches->count; i++) {
        const STRING_FUZZY_MATCH *const match = Vector_Get(matches, i);

        lua_newtable(L);
        lua_pushstring(L, match->key);
        lua_setfield(L, -2, "key");

        lua_rawgeti(L, 2, (int32_t)(intptr_t)match->value);
        M_PushRawField(L, "value");
        lua_remove(L, -2);
        lua_setfield(L, -2, "value");

        lua_pushinteger(L, match->score.score);
        lua_setfield(L, -2, "score");
        lua_pushboolean(L, match->score.is_full);
        lua_setfield(L, -2, "is_full");
        lua_pushboolean(L, match->score.is_word);
        lua_setfield(L, -2, "is_word");

        lua_seti(L, -2, i + 1);
    }

    Vector_Free(matches);
    Vector_Free(sources);
    return 1;
}

// trxc.strings.regex_match(subject, pattern) -> bool
static int M_L_StringsRegexMatch(lua_State *const L)
{
    lua_pushboolean(
        L, String_Match(luaL_checkstring(L, 1), luaL_checkstring(L, 2)));
    return 1;
}

void LUA_CreateStrings(lua_State *const L)
{
    lua_getglobal(L, "trxc");
    lua_newtable(L);
    lua_pushcfunction(L, M_L_StringsFuzzyMatch);
    lua_setfield(L, -2, "fuzzy_match");
    lua_pushcfunction(L, M_L_StringsRegexMatch);
    lua_setfield(L, -2, "regex_match");
    lua_setfield(L, -2, "strings");
    lua_pop(L, 1);
}
