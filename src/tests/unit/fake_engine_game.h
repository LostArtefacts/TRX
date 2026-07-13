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
    int32_t last_num;
} FAKE_GAME_CALLS;

extern FAKE_GAME_CALLS g_FakeGameCalls;

void FakeGame_Reset(void);

// Nothing is being played until something is.
void FakeGame_SetCurrentLevel(int32_t idx);
