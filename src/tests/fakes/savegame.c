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
// save should report failure, and any per-pool slot counts a test has set.
static int32_t m_FreeIndex = -1;
static int32_t m_FreePool = -1;
static bool m_SaveFails;
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

// fake.set_slot_free(index, pool)
static int M_L_SetSlotFree(lua_State *const L)
{
    m_FreeIndex = (int32_t)luaL_checkinteger(L, 1);
    m_FreePool = (int32_t)luaL_checkinteger(L, 2);
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
    lua_pushcfunction(L, M_L_SetSlotCount);
    lua_setfield(L, -2, "set_slot_count");
}
