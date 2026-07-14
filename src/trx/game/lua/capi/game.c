#include <trx/game/game_flow.h>
#include <trx/game/game_flow/types.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/field.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/struct.h>
#include <trx/game/lua/utils.h>
#include <trx/game/savegame.h>
#include <trx/version.h>

#include <lauxlib.h>

static bool M_GetOrdinal(const void *const self, FIELD_VALUE *const out)
{
    const GF_LEVEL *const level = self;
    *out = (FIELD_VALUE) {
        .type = FT_INT32,
        .as_int =
            GF_GetLevelOrdinalNumber(GF_GetLevelTableType(level->type), level),
    };
    return true;
}

// clang-format off
static const FIELD_DESC M_GF_LEVEL_FIELDS[] = {
    FIELD_FN("num", FT_INT32, M_GetOrdinal, nullptr),
    FIELD_RO(GF_LEVEL, type),
    FIELD_RO(GF_LEVEL, path),
    FIELD_RO(GF_LEVEL, title),
    FIELD_RO(GF_LEVEL, script_path),
    FIELD_RO(GF_LEVEL, lara_outfit),
    FIELD_RO(GF_LEVEL, music_track),
    FIELD_RO(GF_LEVEL, water_particles),

    // what the stats screen must not count against the player
    FIELD_RO(GF_LEVEL, unobtainable.pickups),
    FIELD_RO(GF_LEVEL, unobtainable.kills),
    FIELD_RO(GF_LEVEL, unobtainable.ally_kills),
    FIELD_RO(GF_LEVEL, unobtainable.secrets),

    // Every member here is read-only, and deliberately: a level is what the game
    // flow file says it is. The sequence, the injections and the item drops are
    // not exposed at all - they are the level's program and its load-time data,
    // and neither is a contract.
};
// clang-format on

TYPE_DEFINE(GF_LEVEL, M_GF_LEVEL_FIELDS)

// trxc.game.get_version() → int
static int M_L_GameVersion(lua_State *const L)
{
    lua_pushinteger(L, g_TRVersion);
    return 1;
}

// trxc.game.get_trx_version() → string
static int M_L_TRXVersion(lua_State *const L)
{
    lua_pushstring(L, g_TRXVersion);
    return 1;
}

// GF_GetLevelTable indexes the level tables with this and does not check it,
// and GFLT_UNKNOWN is -1, so the range starts at zero.
static GF_LEVEL_TABLE_TYPE M_CheckTableType(lua_State *const L, const int arg)
{
    return (GF_LEVEL_TABLE_TYPE)LUA_CheckRange(
        L, arg, GFLT_NUMBER_OF, "unknown level table");
}

// trxc.game.count_levels() → int
static int M_L_GameCountLevels(lua_State *const L)
{
    const GF_LEVEL_TABLE_TYPE table_type = M_CheckTableType(L, 1);
    lua_pushinteger(L, GF_GetLevelCount(table_type));
    return 1;
}

// A level lives in the game flow for the whole session, so a handle to one
// never goes stale. It is addressed by table and index, which the ref carries
// packed: the table in the high half, the index in the low.
#define M_PACK(table, idx) (((table) << 16) | ((idx) & 0xffff))
#define M_TABLE(packed) ((packed) >> 16)
#define M_IDX(packed) ((packed) & 0xffff)

static void *M_Resolve(const LUA_STRUCT_REF *const ref)
{
    return (void *)GF_GetLevel(M_TABLE(ref->idx), M_IDX(ref->idx));
}

static void M_PushLevel(
    lua_State *const L, const GF_LEVEL_TABLE_TYPE table_type, const int32_t idx)
{
    if (GF_GetLevel(table_type, idx) == nullptr) {
        lua_pushnil(L);
        return;
    }
    LUA_Struct_Push(L, &TYPE_GF_LEVEL, M_Resolve, M_PACK(table_type, idx), 0);
}

// trxc.game.get_level(table_type, idx) -> GF_LEVEL handle or nil
static int M_L_GameGetLevel(lua_State *const L)
{
    const GF_LEVEL_TABLE_TYPE table_type = M_CheckTableType(L, 1);
    // Lua counts from 1, the table from 0.
    M_PushLevel(L, table_type, luaL_checkinteger(L, 2) - 1);
    return 1;
}

// trxc.game.get_current_level() -> GF_LEVEL handle or nil
static int M_L_GameGetCurrentLevel(lua_State *const L)
{
    const GF_LEVEL *const level = GF_GetCurrentLevel();
    if (level == nullptr) {
        lua_pushnil(L);
        return 1;
    }
    const GF_LEVEL_TABLE_TYPE table_type = GF_GetLevelTableType(level->type);
    M_PushLevel(L, table_type, level - GF_GetLevelTable(table_type)->levels);
    return 1;
}

static int M_L_GamePlayLevel(lua_State *const L)
{
    const int32_t level_idx = luaL_checkinteger(L, 1) - 1;
    const int32_t count = GF_GetLevelCount(GFLT_MAIN);
    if (level_idx < 0 || level_idx >= count) {
        return luaL_error(L, "invalid level number: %d", level_idx);
    }
    const GF_LEVEL *const current_level = GF_GetCurrentLevel();
    if (current_level != nullptr) {
        const GF_LEVEL *const next_level = GF_GetLevel(GFLT_MAIN, level_idx);
        if (next_level != nullptr) {
            Savegame_PersistGameToCurrentInfo(next_level);
            RESUME_INFO *const resume = Savegame_GetCurrentInfo(next_level);
            if (resume != nullptr) {
                resume->prev_level = current_level->num;
            }
        }
    }
    GF_OverrideCommand((GF_COMMAND) {
        .action = GF_START_GAME,
        .param = level_idx,
    });
    return 0;
}

// trxc.game.play_cutscene(num) → nil
static int M_L_GamePlayCutscene(lua_State *const L)
{
    const int32_t idx = luaL_checkinteger(L, 1) - 1;
    const int32_t count = GF_GetLevelCount(GFLT_CUTSCENES);
    if (idx < 0 || idx >= count) {
        return luaL_error(L, "invalid cutscene number: %d", idx);
    }
    GF_OverrideCommand((GF_COMMAND) {
        .action = GF_START_CINE,
        .param = idx,
    });
    return 0;
}

// trxc.game.play_demo(num) → nil
static int M_L_GamePlayDemo(lua_State *const L)
{
    const int32_t idx = luaL_checkinteger(L, 1) - 1;
    const int32_t count = GF_GetLevelCount(GFLT_DEMOS);
    if (idx < 0 || idx >= count) {
        return luaL_error(L, "invalid demo number: %d", idx);
    }
    GF_OverrideCommand((GF_COMMAND) {
        .action = GF_START_DEMO,
        .param = idx,
    });
    return 0;
}

static const luaL_Reg m_Module[] = {
    { "get_version", M_L_GameVersion },
    { "get_trx_version", M_L_TRXVersion },
    { "count_levels", M_L_GameCountLevels },
    { "get_level", M_L_GameGetLevel },
    { "get_current_level", M_L_GameGetCurrentLevel },
    { "play_level", M_L_GamePlayLevel },
    { "play_cutscene", M_L_GamePlayCutscene },
    { "play_demo", M_L_GamePlayDemo },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_Struct_Register(
        L, &TYPE_GF_LEVEL, (const luaL_Reg[]) { { nullptr, nullptr } });

    LUA_RegisterModule(L, "game", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
