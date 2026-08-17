#include <trx/config.h>
#include <trx/game/demo.h>
#include <trx/game/game/state.h>
#include <trx/game/game_flow.h>
#include <trx/game/game_flow/types.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/field.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/struct.h>
#include <trx/game/lua/utils.h>
#include <trx/game/savegame.h>
#include <trx/game/screenshot.h>
#include <trx/version.h>

#include <lauxlib.h>

// A level lives in the game flow for the whole session, so a handle to one
// never goes stale. It is addressed by table and number, which the ref carries
// packed: the table in the high half, the number in the low.
#define M_PACK(table, num) (((table) << 16) | ((num) & 0xffff))
#define M_TABLE(packed) ((packed) >> 16)
#define M_NUM(packed) ((packed) & 0xffff)

static bool M_GetOrdinal(const void *const self, TRX_VALUE *const out)
{
    const GF_LEVEL *const level = self;
    *out = (TRX_VALUE) {
        .type = TVT_S32,
        .as_int =
            GF_GetLevelOrdinalNumber(GF_GetLevelTableType(level->type), level),
    };
    return true;
}

// clang-format off
static const FIELD_DESC m_Fields[] = {
    FIELD_FN("num", TVT_S32, M_GetOrdinal, nullptr),
    FIELD_RO(GF_LEVEL, key),
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

TYPE_DEFINE(GF_LEVEL, m_Fields)

// GF_GetLevelTable indexes the level tables with this and does not check it,
// and GFLT_UNKNOWN is -1, so the range starts at zero.
static GF_LEVEL_TABLE_TYPE M_CheckTableType(lua_State *const L, const int arg)
{
    return (GF_LEVEL_TABLE_TYPE)LUA_CheckRange(
        L, arg, GFLT_NUMBER_OF, "unknown level table");
}

// The numbering a script counts and addresses levels by is the game flow's own:
// the one GF_GetLevelCount counts, which leaves out the gym and the levels the
// flow skips. GF_GetLevel takes a different number - the level's place in the
// table - so the two must not be handed to one another.
static const GF_LEVEL *M_GetLevel(
    const GF_LEVEL_TABLE_TYPE table_type, const int32_t num)
{
    // The title level stands on its own rather than sitting in its table, so
    // there is nothing to look a number up in.
    if (table_type == GFLT_TITLE) {
        return num == 1 ? GF_GetTitleLevel() : nullptr;
    }
    return GF_GetLevelByOrdinalNumber(table_type, num);
}

static void *M_Resolve(const LUA_STRUCT_REF *const ref)
{
    return (void *)M_GetLevel(M_TABLE(ref->handle.id), M_NUM(ref->handle.id));
}

static void M_PushLevel(
    lua_State *const L, const GF_LEVEL_TABLE_TYPE table_type, const int32_t num)
{
    if (M_GetLevel(table_type, num) == nullptr) {
        lua_pushnil(L);
        return;
    }
    LUA_Struct_Push(
        L, &TYPE_GF_LEVEL, M_Resolve,
        (TRX_HANDLE) { .id = M_PACK(table_type, num) });
}

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

// trxc.game.count_levels() → int
static int M_L_GameCountLevels(lua_State *const L)
{
    const GF_LEVEL_TABLE_TYPE table_type = M_CheckTableType(L, 1);
    lua_pushinteger(L, GF_GetLevelCount(table_type));
    return 1;
}

// The level a script named, or an error naming the argument it came in on. The
// numbered levels start at 1; the gym, which sits at ordinal 0, is not one of
// them and play_level cannot name it.
static const GF_LEVEL *M_CheckLevel(
    lua_State *const L, const int arg, const GF_LEVEL_TABLE_TYPE table_type,
    const char *const what)
{
    int32_t num;
    const GF_LEVEL *const level =
        LUA_CheckBoundedInt(L, arg, 1, INT32_MAX, &num)
        ? M_GetLevel(table_type, num)
        : nullptr;
    luaL_argcheck(L, level != nullptr, arg, what);
    return level;
}

// trxc.game.get_level(table_type, num) -> GF_LEVEL handle or nil
// Ordinal 0 is the gym, so the range starts there.
static int M_L_GameGetLevel(lua_State *const L)
{
    const GF_LEVEL_TABLE_TYPE table_type = M_CheckTableType(L, 1);
    int32_t num;
    if (!LUA_CheckBoundedInt(L, 2, 0, INT32_MAX, &num)) {
        lua_pushnil(L);
        return 1;
    }
    M_PushLevel(L, table_type, num);
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
    // A gym has no number and reads 0, which is also the number
    // GF_GetLevelByOrdinalNumber hands it back on.
    const int32_t num = table_type == GFLT_TITLE
        ? 1
        : GF_GetLevelOrdinalNumber(table_type, level);
    M_PushLevel(L, table_type, num);
    return 1;
}

// trxc.game.is_loaded() -> bool
static int M_L_GameIsLoaded(lua_State *const L)
{
    lua_pushboolean(L, Game_IsLoaded());
    return 1;
}

// trxc.game.is_playable() -> bool
static int M_L_GameIsPlayable(lua_State *const L)
{
    lua_pushboolean(L, Game_IsPlayable());
    return 1;
}

// trxc.game.is_ngplus() -> bool
static int M_L_GameIsNGPlus(lua_State *const L)
{
    lua_pushboolean(L, Game_IsBonusFlagSet(GBF_NGPLUS));
    return 1;
}

// trxc.game.play_level(num) → nil
// What the game flow's commands carry is the level's place in its table, which
// is GF_LEVEL.num - the same number the `play` console command hands them.
static int M_L_GamePlayLevel(lua_State *const L)
{
    const GF_LEVEL *const next_level =
        M_CheckLevel(L, 1, GFLT_MAIN, "unknown level");

    // `select` starts the level the way the level-select screen and the `play`
    // console command do: Lara's inventory is rebuilt to what she would be
    // carrying on reaching it. Without it the level continues from the one in
    // progress, which is recorded as the previous level.
    bool select = false;
    if (!lua_isnoneornil(L, 2)) {
        luaL_checktype(L, 2, LUA_TTABLE);
        lua_getfield(L, 2, "select");
        select = lua_toboolean(L, -1);
        lua_pop(L, 1);
    }

    if (select) {
        GF_OverrideCommand((GF_COMMAND) {
            .action = GF_SELECT_GAME,
            .param = next_level->num,
        });
        return 0;
    }

    const GF_LEVEL *const current_level = GF_GetCurrentLevel();
    if (current_level != nullptr) {
        SG_Resume_StoreGameToEntry(next_level);
        RESUME_INFO *const resume = SG_Resume_GetEntry(next_level);
        if (resume != nullptr) {
            resume->prev_level = current_level->num;
        }
    }
    GF_OverrideCommand((GF_COMMAND) {
        .action = GF_START_GAME,
        .param = next_level->num,
    });
    return 0;
}

// trxc.game.play_cutscene(num) → nil
static int M_L_GamePlayCutscene(lua_State *const L)
{
    const GF_LEVEL *const level =
        M_CheckLevel(L, 1, GFLT_CUTSCENES, "unknown cutscene");
    GF_OverrideCommand((GF_COMMAND) {
        .action = GF_START_CINE,
        .param = level->num,
    });
    return 0;
}

// trxc.game.play_demo([num]) → GF_LEVEL handle or nil
// With no number, the next demo in rotation plays - the one the attract mode
// would show next, since both draw from Demo_ChooseLevel.
static int M_L_GamePlayDemo(lua_State *const L)
{
    const GF_LEVEL *level;
    if (lua_isnoneornil(L, 1)) {
        const int32_t idx = Demo_ChooseLevel(-1);
        if (idx < 0) {
            lua_pushnil(L);
            return 1;
        }
        level = &GF_GetLevelTable(GFLT_DEMOS)->levels[idx];
    } else {
        level = M_CheckLevel(L, 1, GFLT_DEMOS, "unknown demo");
    }
    GF_OverrideCommand((GF_COMMAND) {
        .action = GF_START_DEMO,
        .param = level->num,
    });
    M_PushLevel(L, GFLT_DEMOS, GF_GetLevelOrdinalNumber(GFLT_DEMOS, level));
    return 1;
}

// trxc.game.play_gym() → nil
//
// A gym has no ordinal, so play_level cannot reach it. It is started through
// GF_SELECT_GAME, as the `gym` console command starts it.
static int M_L_GamePlayGym(lua_State *const L)
{
    const GF_LEVEL *const gym_level = GF_GetGymLevel();
    if (gym_level == nullptr) {
        return luaL_error(L, "this game has no gym");
    }
    GF_OverrideCommand((GF_COMMAND) {
        .action = GF_SELECT_GAME,
        .param = gym_level->num,
    });
    return 0;
}

// trxc.game.end_level()
static int M_L_GameEndLevel(lua_State *const L)
{
    Game_SetIsLevelComplete(true);
    return 0;
}

// trxc.game.exit_to_title()
static int M_L_GameExitToTitle(lua_State *const L)
{
    GF_OverrideCommand((GF_COMMAND) { .action = GF_EXIT_TO_TITLE });
    return 0;
}

// trxc.game.exit_game()
static int M_L_GameExitGame(lua_State *const L)
{
    GF_OverrideCommand((GF_COMMAND) { .action = GF_EXIT_GAME });
    return 0;
}

// trxc.game.screenshot([path])
static int M_L_GameScreenshot(lua_State *const L)
{
    if (lua_isnoneornil(L, 1)) {
        Screenshot_Make(g_Config.rendering.screenshot_format);
    } else {
        Screenshot_MakeToPath(luaL_checkstring(L, 1));
    }
    return 0;
}

static const luaL_Reg m_Module[] = {
    { "get_version", M_L_GameVersion },
    { "get_trx_version", M_L_TRXVersion },
    { "count_levels", M_L_GameCountLevels },
    { "get_level", M_L_GameGetLevel },
    { "get_current_level", M_L_GameGetCurrentLevel },
    { "is_loaded", M_L_GameIsLoaded },
    { "is_playable", M_L_GameIsPlayable },
    { "is_ngplus", M_L_GameIsNGPlus },
    { "play_level", M_L_GamePlayLevel },
    { "play_cutscene", M_L_GamePlayCutscene },
    { "play_demo", M_L_GamePlayDemo },
    { "play_gym", M_L_GamePlayGym },
    { "screenshot", M_L_GameScreenshot },
    { "end_level", M_L_GameEndLevel },
    { "exit_to_title", M_L_GameExitToTitle },
    { "exit_game", M_L_GameExitGame },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_Struct_Register(L, &TYPE_GF_LEVEL, nullptr);
    LUA_RegisterModule(L, "game", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
