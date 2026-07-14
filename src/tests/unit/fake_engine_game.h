#pragma once

#include <trx/game/game_flow/types.h>

#include <lualib.h>
#include <stdint.h>

#define FAKE_LEVEL_COUNT 3
#define FAKE_CUTSCENE_COUNT 1
#define FAKE_DEMO_COUNT 1

// What the surface asked the engine to do, recorded rather than performed.
typedef struct {
    int32_t play_level;
    int32_t play_cutscene;
    int32_t play_demo;
    int32_t play_gym;
    int32_t last_num;
} FAKE_GAME_CALLS;

extern FAKE_GAME_CALLS g_FakeGameCalls;

void FakeGame_Reset(void);

// A negative index leaves the current level unset.
void FakeGame_SetCurrentLevel(int32_t idx);

// Whether the flow has a gym. A game without one has nothing to play.
void FakeGame_SetGymPresent(bool present);

void FakeGame_SetInCutscene(bool in_cutscene);

// The game flow as a test script sees it: fake.set_current_level(),
// fake.set_current_title(), fake.set_in_cutscene(), fake.LEVEL_COUNT and
// fake.NUMBERED_LEVEL_COUNT.
void FakeGame_PushLua(lua_State *L);

// Adds what the game flow was asked to do to the table on top of the stack.
void FakeGame_PushCalls(lua_State *L);
