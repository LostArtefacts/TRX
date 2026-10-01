#pragma once

// Controls what the faked input reports, per role.

#include <trx/game/input.h>

#include <lualib.h>

// Clears every press, hold and hold-off.
void FakeInput_Reset(void);

// Adds fake.press, fake.hold, fake.release_all and fake.held_off to the table
// on top of the stack.
void FakeInput_PushLua(lua_State *L);
