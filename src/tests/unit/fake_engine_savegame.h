#pragma once

#include <lauxlib.h>

#include <stdint.h>

// The save/load calls a command makes, recorded rather than performed. A save
// index of -1 stands for "none given" - the next quick slot.
typedef struct {
    int32_t save_count;
    int32_t save_index;
    int32_t save_pool;
    int32_t load_count;
    int32_t load_index;
    int32_t load_pool;
} FAKE_SAVEGAME_CALLS;

extern FAKE_SAVEGAME_CALLS g_FakeSavegameCalls;

void FakeSavegame_Reset(void);
void FakeSavegame_PushCalls(lua_State *L);
// The knobs a test turns before it runs a command: fake.set_slot_free(index,
// pool), fake.set_save_fails(bool) and fake.set_slot_count(pool, n). By default
// every slot is taken (a load finds a game), every save writes, and each pool
// holds ten slots.
void FakeSavegame_PushLua(lua_State *L);
