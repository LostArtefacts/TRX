#pragma once

#include <trx/game/gym.h>

#include <stdint.h>

typedef enum {
    FAKE_ASSAULT_TIMING_PENALTY,
    FAKE_ASSAULT_TIMING_TARGET_PENALTY,
    FAKE_ASSAULT_TIMING_PENALTY_TIMER,
    FAKE_ASSAULT_TIMING_LAP_TIME,
    FAKE_ASSAULT_TIMING_LAP_TIMER,
    FAKE_ASSAULT_TIMING_NUMBER_OF,
} FAKE_ASSAULT_TIMING;

// Outside a gym level the timers do not exist, and every verb raises.
void FakeAssault_SetInGym(bool in_gym);
void FakeAssault_SetHasStats(GYM_TRACK_TYPE track, bool has_stats);
void FakeAssault_SetRunning(GYM_TRACK_TYPE track, bool running);
void FakeAssault_SetVisible(GYM_TRACK_TYPE track, bool visible);
void FakeAssault_SetActiveTrack(GYM_TRACK_TYPE track);
void FakeAssault_SetTiming(
    GYM_TRACK_TYPE track, FAKE_ASSAULT_TIMING timing, int32_t frames);
