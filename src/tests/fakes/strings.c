// A pcre2-free stand-in for trxc.strings, so a command's routing test can match
// names without linking the regex engine. The matching is a plain
// case-insensitive substring, best (earliest, then shortest key) first - enough
// to route "caves" to Caves. The real matcher is exercised in lua_strings and
// lua_argparse, which link pcre2 on purpose.

#include <trx/game/lua/common.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>

#include <ctype.h>
#include <lauxlib.h>
#include <string.h>

static bool M_CaseSubstr(
    const char *const hay, const char *const needle, int32_t *const pos)
{
    if (needle[0] == '\0') {
        *pos = 0;
        return true;
    }
    for (int32_t i = 0; hay[i] != '\0'; i++) {
        int32_t j = 0;
        while (needle[j] != '\0'
               && tolower((unsigned char)hay[i + j])
                   == tolower((unsigned char)needle[j])) {
            j++;
        }
        if (needle[j] == '\0') {
            *pos = i;
            return true;
        }
    }
    return false;
}

static bool M_CaseEqual(const char *a, const char *b)
{
    while (*a != '\0' && *b != '\0') {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
            return false;
        }
        a++;
        b++;
    }
    return *a == *b;
}

// trxc.strings.fuzzy_match(input, sources) -> { { key, value, score, is_full,
// is_word }, ... }, best first.
static int M_L_FuzzyMatch(lua_State *const L)
{
    const char *const input = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);
    const int32_t count = (int32_t)lua_rawlen(L, 2);

    enum { MAX = 512 };
    int32_t idx[MAX];
    int32_t score[MAX];
    int32_t found = 0;

    for (int32_t i = 1; i <= count && found < MAX; i++) {
        lua_rawgeti(L, 2, i);
        lua_getfield(L, -1, "key");
        const char *const key = lua_tostring(L, -1);
        int32_t pos;
        if (key != nullptr && M_CaseSubstr(key, input, &pos)) {
            idx[found] = i;
            score[found] = 1000 - pos * 10 - (int32_t)strlen(key);
            found++;
        }
        lua_pop(L, 2);
    }

    for (int32_t a = 0; a < found; a++) {
        for (int32_t b = a + 1; b < found; b++) {
            if (score[b] > score[a]) {
                const int32_t ts = score[a];
                score[a] = score[b];
                score[b] = ts;
                const int32_t ti = idx[a];
                idx[a] = idx[b];
                idx[b] = ti;
            }
        }
    }

    lua_newtable(L);
    for (int32_t k = 0; k < found; k++) {
        lua_rawgeti(L, 2, idx[k]);

        lua_getfield(L, -1, "key");
        const bool is_full = M_CaseEqual(lua_tostring(L, -1), input);
        lua_pop(L, 1);

        lua_newtable(L);
        lua_getfield(L, -2, "key");
        lua_setfield(L, -2, "key");
        lua_getfield(L, -2, "value");
        lua_setfield(L, -2, "value");
        lua_pushinteger(L, score[k]);
        lua_setfield(L, -2, "score");
        lua_pushboolean(L, is_full);
        lua_setfield(L, -2, "is_full");
        lua_pushboolean(L, false);
        lua_setfield(L, -2, "is_word");

        lua_remove(L, -2);
        lua_seti(L, -2, k + 1);
    }
    return 1;
}

// trxc.strings.regex_match(subject, pattern) -> bool. No command routes on a
// regex, so the stand-in never matches.
static int M_L_RegexMatch(lua_State *const L)
{
    luaL_checkstring(L, 1);
    luaL_checkstring(L, 2);
    lua_pushboolean(L, false);
    return 1;
}

static const luaL_Reg m_Module[] = {
    { "fuzzy_match", M_L_FuzzyMatch },
    { "regex_match", M_L_RegexMatch },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "strings", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
