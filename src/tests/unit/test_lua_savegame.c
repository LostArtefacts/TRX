// The savegame surface. The assertions live in
// src/tests/unit/lua/savegame.lua.
//
// The fake below stands in for the save store: a normal pool of three slots
// with the first one taken, and two quick saves on disk.

#include "lua_surface.h"

#include <trx/game/game_flow.h>
#include <trx/game/savegame.h>

int32_t Savegame_GetSlotCount(const SAVEGAME_SLOT_POOL pool)
{
    return pool == SAVEGAME_SLOT_POOL_QUICK ? 4 : 3;
}

int32_t Savegame_GetQuickVisualCount(void)
{
    return 2;
}

SAVEGAME_SLOT_REF Savegame_NormalSlot(const int32_t index)
{
    return (SAVEGAME_SLOT_REF) { .pool = SAVEGAME_SLOT_POOL_NORMAL,
                                 .index = index };
}

SAVEGAME_SLOT_REF Savegame_QuickFromVisualIndex(const int32_t visual_index)
{
    return (SAVEGAME_SLOT_REF) { .pool = SAVEGAME_SLOT_POOL_QUICK,
                                 .index = visual_index };
}

// The first normal slot holds a save; every other slot is empty.
bool Savegame_IsSlotFree(const SAVEGAME_SLOT_REF slot)
{
    return !(slot.pool == SAVEGAME_SLOT_POOL_NORMAL && slot.index == 0);
}

int32_t Savegame_SlotToParam(const SAVEGAME_SLOT_REF slot)
{
    return slot.index;
}

static int32_t m_LoadedParam = -1;

void GF_OverrideCommand(const GF_COMMAND command)
{
    m_LoadedParam = command.param;
}

static int M_FakeReset(lua_State *const L)
{
    m_LoadedParam = -1;
    return 0;
}

static int M_FakeCalls(lua_State *const L)
{
    lua_newtable(L);
    lua_pushinteger(L, m_LoadedParam);
    lua_setfield(L, -2, "loaded_param");
    return 1;
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "savegame",
        .tests = "savegame",
        .seal = true,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
