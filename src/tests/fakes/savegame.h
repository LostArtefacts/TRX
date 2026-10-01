#pragma once

#include <lauxlib.h>

#include <stdint.h>

// The knobs a test turns before it runs a command: fake.set_slot_free(index,
// pool), fake.set_save_fails(bool), fake.set_restart_available(bool) and
// fake.set_slot_count(pool, n). By default every slot is taken (a load finds a
// game), every save writes, the running save allows a restart, and each pool
// holds ten slots.
void FakeSavegame_PushLua(lua_State *L);
