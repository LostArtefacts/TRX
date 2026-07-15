#pragma once

#include <trx/game/game_flow/types.h>

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

// The title level, which is not in any table.
void FakeGame_SetCurrentTitle(void);

// Whether the flow has a gym. A game without one has nothing to play.
void FakeGame_SetGymPresent(bool present);
