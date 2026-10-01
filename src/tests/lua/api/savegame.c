// The savegame surface. The assertions live in
// savegame.lua.
//
// The fake below stands in for the save store: a normal pool of three slots
// with the first one taken and two quick saves on disk. The first quick save
// is readable.

#include <harness/fake_calls.h>
#include <harness/lua_surface.h>

#include <trx/game/game_flow.h>
#include <trx/game/savegame.h>

static SAVEGAME_SLOT_REF m_SavedSlot = { .index = -1 };

static int32_t m_LoadedParam = -1;

// The slot the fake ended up holding is state, not a call, so it is read here.
static int M_FakeReset(lua_State *const L)
{
    FakeCalls_Reset();
    m_LoadedParam = -1;
    m_SavedSlot = (SAVEGAME_SLOT_REF) { .index = -1 };
    return 0;
}

static int M_FakeCalls(lua_State *const L)
{
    FakeCalls_Push(L);
    lua_pushinteger(L, m_LoadedParam);
    lua_setfield(L, -2, "loaded_param");
    lua_pushinteger(L, m_SavedSlot.index);
    lua_setfield(L, -2, "saved_index");
    lua_pushinteger(L, m_SavedSlot.pool);
    lua_setfield(L, -2, "saved_pool");
    return 1;
}

int32_t SG_Manager_GetSlotCount(const SAVEGAME_SLOT_POOL pool)
{
    return pool == SAVEGAME_SLOT_POOL_QUICK ? 4 : 3;
}

int32_t SG_Manager_GetQuickVisualCount(void)
{
    return 2;
}

SAVEGAME_SLOT_REF SG_Manager_NormalSlot(const int32_t index)
{
    return (SAVEGAME_SLOT_REF) { .pool = SAVEGAME_SLOT_POOL_NORMAL,
                                 .index = index };
}

SAVEGAME_SLOT_REF SG_Manager_QuickFromVisualIndex(const int32_t visual_index)
{
    return (SAVEGAME_SLOT_REF) { .pool = SAVEGAME_SLOT_POOL_QUICK,
                                 .index = visual_index };
}

// The first normal slot and the first quick slot hold saves. Every other slot
// is empty.
bool SG_Manager_IsSlotFree(const SAVEGAME_SLOT_REF slot)
{
    return slot.index != 0;
}

// Return the save in a taken slot.
const SAVEGAME_INFO *SG_Manager_GetSavegameInfo(const SAVEGAME_SLOT_REF slot)
{
    static SAVEGAME_INFO info = {
        .counter = 7,
        .level_num = 2,
        .level_title = "City of Vilcabamba",
        .features = { .restart = true, .select_level = true },
    };
    return &info;
}

bool SG_Manager_Delete(const SAVEGAME_SLOT_REF slot)
{
    FAKE_RECORD("delete_save", FV(slot.pool), FV(slot.index));
    return true;
}

// Report whether the slot has an available story.
bool GF_HasAvailableStory(const SAVEGAME_SLOT_REF slot)
{
    return slot.pool == SAVEGAME_SLOT_POOL_NORMAL;
}

bool Savegame_IsManualSaveAllowed(void)
{
    return true;
}

// Return the second numbered slot as the most recently used slot.
SAVEGAME_SLOT_REF SG_Manager_GetMostRecentlyUsedSlot(void)
{
    return (SAVEGAME_SLOT_REF) { .pool = SAVEGAME_SLOT_POOL_NORMAL,
                                 .index = 1 };
}

SAVEGAME_SLOT_REF SG_Manager_GetMostRecentlyCreatedSlot(void)
{
    return (SAVEGAME_SLOT_REF) { .pool = SAVEGAME_SLOT_POOL_NORMAL,
                                 .index = 1 };
}

int32_t SG_Manager_QuickToVisualIndex(const SAVEGAME_SLOT_REF slot)
{
    return slot.index;
}

int32_t SG_Manager_GetTotalCount(void)
{
    return 1;
}

SAVEGAME_SLOT_REF SG_Manager_GetBoundSlot(void)
{
    return (SAVEGAME_SLOT_REF) { .pool = SAVEGAME_SLOT_POOL_NORMAL,
                                 .index = 0 };
}

bool Savegame_RestartAvailable(const SAVEGAME_SLOT_REF slot)
{
    return true;
}

// Add the gym and first level to the reached-level list.
RESULT Savegame_ReadReachedLevels(
    const SAVEGAME_SLOT_REF slot, VECTOR *const levels)
{
    static const GF_LEVEL gym = { .num = 0, .type = GFL_GYM };
    static const GF_LEVEL first = { .num = 1, .type = GFL_NORMAL };
    const GF_LEVEL *level = &gym;
    Vector_Add(levels, &level);
    level = &first;
    Vector_Add(levels, &level);
    return OK;
}

int32_t GF_GetLevelOrdinalNumber(
    const GF_LEVEL_TABLE_TYPE level_table_type, const GF_LEVEL *const level)
{
    return level->type == GFL_GYM ? 0 : level->num;
}

int32_t SG_Manager_SlotToParam(const SAVEGAME_SLOT_REF slot)
{
    return slot.index;
}

SAVEGAME_SLOT_REF SG_Manager_GetNextQuickSlot(void)
{
    return (SAVEGAME_SLOT_REF) { .pool = SAVEGAME_SLOT_POOL_QUICK, .index = 3 };
}

bool SG_Manager_IsValidSlotRef(const SAVEGAME_SLOT_REF slot)
{
    return slot.index >= 0;
}

bool Savegame_Save(const SAVEGAME_SLOT_REF slot)
{
    m_SavedSlot = slot;
    return true;
}

void GF_OverrideCommand(const GF_COMMAND command, const bool immediate)
{
    m_LoadedParam = command.param;
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .fake_reset = M_FakeReset,
        .module = "savegame",
        .tests = "api/savegame",
        .seal = true,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
