// trxc.savegame, reduced to what save/load route through: the pool and index of
// each call, recorded rather than performed. Ten slots in each pool, all taken,
// so a load always finds a game and a numbered save is in range.

#include <fakes/savegame.h>

#include <harness/fake_calls.h>

#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>

#include <lauxlib.h>

#define M_DEFAULT_SLOT_COUNT 10
#define M_MAX_POOLS 4

// The one slot a test has marked free (index -1 means none), whether the next
// save should report failure, whether the running save allows a restart, and
// any per-pool slot counts a test has set.
static int32_t m_FreeIndex = -1;
static int32_t m_FreePool = -1;
static bool m_SaveFails;
static bool m_RestartAvailable = true;
static int32_t m_PoolIds[M_MAX_POOLS];
static int32_t m_PoolCounts[M_MAX_POOLS];
static int32_t m_PoolN;

static int32_t M_SlotCount(const int32_t pool)
{
    for (int32_t i = 0; i < m_PoolN; i++) {
        if (m_PoolIds[i] == pool) {
            return m_PoolCounts[i];
        }
    }
    return M_DEFAULT_SLOT_COUNT;
}

static void M_Reset(void)
{
    m_FreeIndex = -1;
    m_FreePool = -1;
    m_SaveFails = false;
    m_RestartAvailable = true;
    m_PoolN = 0;
}

FAKE_ON_RESET(M_Reset)

// trxc.savegame.slot_count(pool) -> int
static int M_L_SlotCount(lua_State *const L)
{
    lua_pushinteger(L, M_SlotCount((int32_t)luaL_checkinteger(L, 1)));
    return 1;
}

// trxc.savegame.is_free(index, pool) -> bool. Taken unless a test freed it.
static int M_L_IsFree(lua_State *const L)
{
    const int32_t index = (int32_t)luaL_checkinteger(L, 1);
    const int32_t pool = (int32_t)luaL_checkinteger(L, 2);
    lua_pushboolean(L, index == m_FreeIndex && pool == m_FreePool);
    return 1;
}

// trxc.savegame.load(index, pool)
static int M_L_Load(lua_State *const L)
{
    const int32_t index = (int32_t)luaL_checkinteger(L, 1);
    const int32_t pool = (int32_t)luaL_checkinteger(L, 2);
    FAKE_RECORD("load", FV(index), FV(pool));
    return 0;
}

// trxc.savegame.save(index, pool) -> bool
static int M_L_Save(lua_State *const L)
{
    const int32_t index =
        lua_isnoneornil(L, 1) ? -1 : (int32_t)luaL_checkinteger(L, 1);
    const int32_t pool = (int32_t)luaL_checkinteger(L, 2);
    FAKE_RECORD("save", FV(index), FV(pool));
    lua_pushboolean(L, !m_SaveFails);
    return 1;
}

// trxc.savegame.total_count() -> int
static int M_L_TotalCount(lua_State *const L)
{
    lua_pushinteger(L, M_DEFAULT_SLOT_COUNT);
    return 1;
}

// trxc.savegame.restart_available(index, pool) -> bool
static int M_L_RestartAvailable(lua_State *const L)
{
    lua_pushboolean(L, m_RestartAvailable);
    return 1;
}

// trxc.savegame.recent_slot() -> index, pool
static int M_L_RecentSlot(lua_State *const L)
{
    lua_pushinteger(L, 1);
    lua_pushinteger(L, 0);
    return 2;
}

// trxc.savegame.manual_allowed() -> bool
static int M_L_ManualAllowed(lua_State *const L)
{
    lua_pushboolean(L, true);
    return 1;
}

// trxc.savegame.info(index, pool) -> table or nil
static int M_L_Info(lua_State *const L)
{
    const int32_t index = (int32_t)luaL_checkinteger(L, 1);
    const int32_t pool = (int32_t)luaL_checkinteger(L, 2);
    if (index == m_FreeIndex && pool == m_FreePool) {
        lua_pushnil(L);
        return 1;
    }
    lua_newtable(L);
    lua_pushstring(L, "City of Vilcabamba");
    lua_setfield(L, -2, "level_title");
    lua_pushinteger(L, index);
    lua_setfield(L, -2, "counter");
    // A level number below the reached levels, as a game with no gym gives.
    lua_pushinteger(L, 1);
    lua_setfield(L, -2, "level_num");
    lua_pushboolean(L, pool != 0);
    lua_setfield(L, -2, "is_quick");
    lua_pushboolean(L, true);
    lua_setfield(L, -2, "can_restart");
    lua_pushboolean(L, true);
    lua_setfield(L, -2, "can_select_level");
    lua_pushboolean(L, true);
    lua_setfield(L, -2, "has_story");
    return 1;
}

// trxc.savegame.delete(index, pool) -> bool
static int M_L_Delete(lua_State *const L)
{
    FAKE_RECORD(
        "delete", FV((int32_t)luaL_checkinteger(L, 1)),
        FV((int32_t)luaL_checkinteger(L, 2)));
    lua_pushboolean(L, true);
    return 1;
}

// trxc.savegame.play_story(index, pool)
static int M_L_PlayStory(lua_State *const L)
{
    FAKE_RECORD(
        "play_story", FV((int32_t)luaL_checkinteger(L, 1)),
        FV((int32_t)luaL_checkinteger(L, 2)));
    return 0;
}

// trxc.savegame.reached_levels(index, pool) -> {int}. The first two levels.
static int M_L_ReachedLevels(lua_State *const L)
{
    FAKE_RECORD(
        "reached_levels", FV((int32_t)luaL_checkinteger(L, 1)),
        FV((int32_t)luaL_checkinteger(L, 2)));
    lua_createtable(L, 2, 0);
    for (int32_t i = 1; i <= 2; i++) {
        lua_pushinteger(L, i);
        lua_rawseti(L, -2, i);
    }
    return 1;
}

// fake.set_slot_free(index, pool)
static int M_L_SetSlotFree(lua_State *const L)
{
    m_FreeIndex = (int32_t)luaL_checkinteger(L, 1);
    m_FreePool = (int32_t)luaL_checkinteger(L, 2);
    return 0;
}

// fake.set_restart_available(bool)
static int M_L_SetRestartAvailable(lua_State *const L)
{
    m_RestartAvailable = lua_toboolean(L, 1);
    return 0;
}

// fake.set_save_fails(bool)
static int M_L_SetSaveFails(lua_State *const L)
{
    m_SaveFails = lua_toboolean(L, 1);
    return 0;
}

// fake.set_slot_count(pool, n)
static int M_L_SetSlotCount(lua_State *const L)
{
    const int32_t pool = (int32_t)luaL_checkinteger(L, 1);
    const int32_t count = (int32_t)luaL_checkinteger(L, 2);
    for (int32_t i = 0; i < m_PoolN; i++) {
        if (m_PoolIds[i] == pool) {
            m_PoolCounts[i] = count;
            return 0;
        }
    }
    if (m_PoolN < M_MAX_POOLS) {
        m_PoolIds[m_PoolN] = pool;
        m_PoolCounts[m_PoolN] = count;
        m_PoolN++;
    }
    return 0;
}

static const luaL_Reg m_Module[] = {
    { "slot_count", M_L_SlotCount },
    { "is_free", M_L_IsFree },
    { "total_count", M_L_TotalCount },
    { "restart_available", M_L_RestartAvailable },
    { "recent_slot", M_L_RecentSlot },
    { "manual_allowed", M_L_ManualAllowed },
    { "info", M_L_Info },
    { "delete", M_L_Delete },
    { "play_story", M_L_PlayStory },
    { "reached_levels", M_L_ReachedLevels },
    { "load", M_L_Load },
    { "save", M_L_Save },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "savegame", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)

void FakeSavegame_PushLua(lua_State *const L)
{
    lua_pushcfunction(L, M_L_SetSlotFree);
    lua_setfield(L, -2, "set_slot_free");
    lua_pushcfunction(L, M_L_SetSaveFails);
    lua_setfield(L, -2, "set_save_fails");
    lua_pushcfunction(L, M_L_SetRestartAvailable);
    lua_setfield(L, -2, "set_restart_available");
    lua_pushcfunction(L, M_L_SetSlotCount);
    lua_setfield(L, -2, "set_slot_count");
}
