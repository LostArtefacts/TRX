#pragma once

#include <trx/game/game_flow/types.h>

#include <lualib.h>
#include <stdint.h>

#define FAKE_LEVEL_COUNT 3
#define FAKE_CUTSCENE_COUNT 1
#define FAKE_DEMO_COUNT 1
#define FAKE_FMV_COUNT 2

// A negative index leaves the current level unset.
void FakeGame_SetCurrentLevel(int32_t idx);

// Whether the flow has a gym. A game without one has nothing to play.
void FakeGame_SetGymPresent(bool present);

void FakeGame_SetInCutscene(bool in_cutscene);

// Whether this run started from the passport's bonus entry.
void FakeGame_SetNGPlus(bool ngplus);

// The game flow as a test script sees it: fake.set_current_level(),
// fake.set_current_title(), fake.set_in_cutscene(), fake.LEVEL_COUNT and
// fake.NUMBERED_LEVEL_COUNT.
void FakeGame_PushLua(lua_State *L);
