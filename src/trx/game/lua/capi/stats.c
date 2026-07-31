#include <trx/game/game/state.h>
#include <trx/game/game_flow.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/field.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/struct.h>
#include <trx/game/lua/utils.h>
#include <trx/game/stats.h>

#include <lauxlib.h>

// Secrets are addressed by the number the player says, counted from one; the
// bit index behind it stays in the engine.

// A level lives in the game flow for the whole session, so what addresses its
// statistics never goes stale. The ref carries the table in the high half and
// the level's number in the low, the way a level handle does; a category
// handle carries a category id below that again.
#define M_PACK_LEVEL(table, num) (((table) << 16) | ((num) & 0xffff))
#define M_LEVEL_TABLE(packed) (((packed) >> 16) & 0xff)
#define M_LEVEL_NUM(packed) ((packed) & 0xffff)
#define M_PACK_CAT(level, id) (((level) << 8) | (id))
#define M_CAT_LEVEL(packed) ((packed) >> 8)
#define M_CAT_ID(packed) ((packed) & 0xff)

// The count is the level's, and the view is a copy of it, so a write goes back
// to where it came from rather than into the copy.
static const char *M_SetCategoryCount(
    void *const self, const TRX_VALUE *const in)
{
    const STATS_CATEGORY *const category = self;
    if (!Stats_SetCategoryCount(category->level, category->id, in->as_int)) {
        return "this count is not the level's to set";
    }
    return nullptr;
}

// clang-format off
static const FIELD_DESC m_StatsFields[] = {
    FIELD(LEVEL_STATS, timer),
    FIELD(LEVEL_STATS, death_count),
    FIELD(LEVEL_STATS, ammo_used),
    FIELD(LEVEL_STATS, ammo_hits),
    FIELD(LEVEL_STATS, distance_travelled),
    FIELD(LEVEL_STATS, medipacks_used),

    // What the level is counted on is reached a category at a time, so the
    // counts behind it are not members here. The secret mask is not one
    // either: which secrets Lara holds is what the secret verbs answer.
};

static const FIELD_DESC m_CategoryFields[] = {
    FIELD_SET(STATS_CATEGORY, count, M_SetCategoryCount),
    FIELD_RO(STATS_CATEGORY, max),
    FIELD_RO(STATS_CATEGORY, raw),
    FIELD_RO(STATS_CATEGORY, unobtainable),
};
// clang-format on

// The level a handle names, or nullptr if the game flow no longer has one.
static const GF_LEVEL *M_GetPackedLevel(const int32_t packed)
{
    return GF_GetLevelByOrdinalNumber(
        (GF_LEVEL_TABLE_TYPE)M_LEVEL_TABLE(packed), M_LEVEL_NUM(packed));
}

TYPE_DEFINE(LEVEL_STATS, m_StatsFields)
TYPE_DEFINE(STATS_CATEGORY, m_CategoryFields)

static void *M_ResolveStats(const LUA_STRUCT_REF *const ref)
{
    return Stats_GetLevelStats(M_GetPackedLevel(ref->handle.id));
}

// The view a category handle stands for, refilled every time the handle is
// read: it holds a level's counters at the moment it is asked for, and the
// level goes on counting.
static void *M_ResolveCategory(const LUA_STRUCT_REF *const ref)
{
    static STATS_CATEGORY category;
    const GF_LEVEL *const level = M_GetPackedLevel(M_CAT_LEVEL(ref->handle.id));
    return Stats_GetCategory(
               level, (STATS_CATEGORY_ID)M_CAT_ID(ref->handle.id), &category)
        ? &category
        : nullptr;
}

// The level a script means, packed, or -1 where it has no statistics: the
// title screen and the cutscenes. The statistics belong to the level being
// played rather than the one the game flow is on, and the title screen has the
// second without the first.
static int32_t M_PackLevel(const GF_LEVEL *const level)
{
    if (Stats_GetLevelStats(level) == nullptr) {
        return -1;
    }
    const GF_LEVEL_TABLE_TYPE table_type = GF_GetLevelTableType(level->type);
    return M_PACK_LEVEL(
        table_type, GF_GetLevelOrdinalNumber(table_type, level));
}

static void M_PushStats(lua_State *const L, const GF_LEVEL *const level)
{
    const int32_t packed = M_PackLevel(level);
    if (packed < 0) {
        lua_pushnil(L);
        return;
    }
    LUA_Struct_Push(
        L, &TYPE_LEVEL_STATS, M_ResolveStats, (TRX_HANDLE) { .id = packed });
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

// The level a statistics handle was taken against.
static const GF_LEVEL *M_CheckStatsLevel(lua_State *const L, const int arg)
{
    const LUA_STRUCT_REF *const ref =
        LUA_Struct_CheckRef(L, arg, &TYPE_LEVEL_STATS);
    return M_GetPackedLevel(ref->handle.id);
}

// trxc.stats.get(num) -> LEVEL_STATS handle or nil
static int M_L_StatsGet(lua_State *const L)
{
    int32_t num;
    if (!LUA_CheckBoundedInt(L, 1, 0, INT32_MAX, &num)) {
        lua_pushnil(L);
        return 1;
    }
    M_PushStats(L, GF_GetLevelByOrdinalNumber(GFLT_MAIN, num));
    return 1;
}

// trxc.stats.get_current() -> LEVEL_STATS handle or nil
static int M_L_StatsGetCurrent(lua_State *const L)
{
    M_PushStats(L, Game_GetCurrentLevel());
    return 1;
}

// trxc.stats.category(stats, id) -> STATS_CATEGORY handle or nil
static int M_L_StatsCategory(lua_State *const L)
{
    const LUA_STRUCT_REF *const ref =
        LUA_Struct_CheckRef(L, 1, &TYPE_LEVEL_STATS);
    const STATS_CATEGORY_ID id = (STATS_CATEGORY_ID)LUA_CheckRange(
        L, 2, STATS_CAT_NUMBER_OF, "unknown statistics category");

    STATS_CATEGORY category;
    if (!Stats_GetCategory(M_GetPackedLevel(ref->handle.id), id, &category)) {
        lua_pushnil(L);
        return 1;
    }
    LUA_Struct_Push(
        L, &TYPE_STATS_CATEGORY, M_ResolveCategory,
        (TRX_HANDLE) { .id = M_PACK_CAT(ref->handle.id, id) });
    return 1;
}

// trxc.stats.kill_split(stats) -> allies, enemies
static int M_L_StatsKillSplit(lua_State *const L)
{
    const GF_LEVEL *const level = M_CheckStatsLevel(L, 1);
    if (!Stats_HasLevelMaxStats(level)) {
        return 0;
    }
    const LEVEL_MAX_STATS *const max_stats = Stats_GetLevelMaxStats(level);
    lua_pushinteger(L, max_stats->max_kill_ally_count);
    lua_pushinteger(L, max_stats->max_kill_non_ally_count);
    return 2;
}

// trxc.stats.allies_hurt(stats) -> bool
static int M_L_StatsAlliesHurt(lua_State *const L)
{
    lua_pushboolean(L, Stats_HaveAlliesBeenHurt(M_CheckStatsLevel(L, 1)));
    return 1;
}

// stats:secret_list() -> { { num=, found= }, ... }
static int M_L_StatsSecretList(lua_State *const L)
{
    const GF_LEVEL *const level = M_CheckStatsLevel(L, 1);
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

// stats:give_secret(num) -> bool
static int M_L_StatsGiveSecret(lua_State *const L)
{
    const GF_LEVEL *const level = M_CheckStatsLevel(L, 1);
    const int16_t secret_idx = M_GetSecretIdx(L, 2);
    lua_pushboolean(L, secret_idx >= 0 && Stats_AddSecret(level, secret_idx));
    return 1;
}

// stats:take_secret(num) -> bool
static int M_L_StatsTakeSecret(lua_State *const L)
{
    const GF_LEVEL *const level = M_CheckStatsLevel(L, 1);
    const int16_t secret_idx = M_GetSecretIdx(L, 2);
    lua_pushboolean(
        L, secret_idx >= 0 && Stats_RemoveSecret(level, secret_idx));
    return 1;
}

static const luaL_Reg m_StatsMethods[] = {
    { "secret_list", M_L_StatsSecretList },
    { "give_secret", M_L_StatsGiveSecret },
    { "take_secret", M_L_StatsTakeSecret },
    { nullptr, nullptr },
};

static const luaL_Reg m_Module[] = {
    { "get", M_L_StatsGet },
    { "get_current", M_L_StatsGetCurrent },
    { "category", M_L_StatsCategory },
    { "kill_split", M_L_StatsKillSplit },
    { "allies_hurt", M_L_StatsAlliesHurt },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_Struct_Register(L, &TYPE_LEVEL_STATS, m_StatsMethods);
    LUA_Struct_Register(L, &TYPE_STATS_CATEGORY, nullptr);
    LUA_RegisterModule(L, "stats", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
