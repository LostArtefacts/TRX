#include <trx/game/game/state.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>
#include <trx/game/stats.h>

#include <lauxlib.h>

// Secrets are addressed by the number the player says, counted from one; the
// bit index behind it stays in the engine.

// The level the statistics are read against, which is the one being played
// rather than the one the game flow is on: the title screen has the second
// without the first.
static const GF_LEVEL *M_GetLevel(void)
{
    return Game_GetCurrentLevel();
}

// The secret a number names, as an index, or -1 when the number is outside the
// range the engine keeps secrets in.
static int16_t M_GetSecretIdx(lua_State *const L, const int arg)
{
    const lua_Integer num = luaL_checkinteger(L, arg);
    if (num < 1 || num > STATS_MAX_SECRETS) {
        return -1;
    }
    return (int16_t)(num - 1);
}

// trxc.stats.secrets() -> { { num=, found= }, ... }
static int M_L_StatsSecrets(lua_State *const L)
{
    const GF_LEVEL *const level = M_GetLevel();
    lua_newtable(L);

    int32_t out_idx = 1;
    for (int16_t i = 0; i < STATS_MAX_SECRETS; i++) {
        if (!Stats_IsSecretValid(level, i)) {
            continue;
        }
        lua_newtable(L);
        lua_pushinteger(L, i + 1);
        lua_setfield(L, -2, "num");
        lua_pushboolean(L, Stats_HasSecret(level, i));
        lua_setfield(L, -2, "found");
        lua_seti(L, -2, out_idx);
        out_idx++;
    }
    return 1;
}

// trxc.stats.secret_count() -> int
static int M_L_StatsSecretCount(lua_State *const L)
{
    const LEVEL_STATS *const stats = Stats_GetLevelStats(M_GetLevel());
    lua_pushinteger(L, stats == nullptr ? 0 : stats->counts[STATS_CAT_SECRETS]);
    return 1;
}

// trxc.stats.max_secret_count() -> int
static int M_L_StatsMaxSecretCount(lua_State *const L)
{
    const GF_LEVEL *const level = M_GetLevel();
    lua_pushinteger(
        L,
        Stats_HasLevelMaxStats(level)
            ? Stats_GetLevelMaxStats(level)->maxes[STATS_CAT_SECRETS]
            : 0);
    return 1;
}

// trxc.stats.give_secret(num) -> bool
static int M_L_StatsGiveSecret(lua_State *const L)
{
    const int16_t secret_idx = M_GetSecretIdx(L, 1);
    lua_pushboolean(
        L, secret_idx >= 0 && Stats_AddSecret(M_GetLevel(), secret_idx));
    return 1;
}

// trxc.stats.take_secret(num) -> bool
static int M_L_StatsTakeSecret(lua_State *const L)
{
    const int16_t secret_idx = M_GetSecretIdx(L, 1);
    lua_pushboolean(
        L, secret_idx >= 0 && Stats_RemoveSecret(M_GetLevel(), secret_idx));
    return 1;
}

static const luaL_Reg m_Module[] = {
    { "secrets", M_L_StatsSecrets },
    { "secret_count", M_L_StatsSecretCount },
    { "max_secret_count", M_L_StatsMaxSecretCount },
    { "give_secret", M_L_StatsGiveSecret },
    { "take_secret", M_L_StatsTakeSecret },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "stats", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
