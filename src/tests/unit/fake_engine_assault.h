#pragma once

#include <trx/game/gym.h>

#include <stdint.h>

// What the surface asked the track manager to do, recorded rather than
// performed. The track is remembered because every verb takes one, and getting
// the default wrong is the easiest mistake for a declaration to make.
typedef struct {
    int32_t start;
    int32_t stop;
    int32_t reset;
    int32_t finish;
    GYM_TRACK_TYPE last_track;
    int32_t config_writes;
} FAKE_ASSAULT_CALLS;

extern FAKE_ASSAULT_CALLS g_FakeAssaultCalls;

void FakeAssault_Reset(void);

// Outside a gym level the timers do not exist, and every verb raises.
void FakeAssault_SetInGym(bool in_gym);
void FakeAssault_SetHasStats(GYM_TRACK_TYPE track, bool has_stats);
void FakeAssault_SetRunning(GYM_TRACK_TYPE track, bool running);
void FakeAssault_SetVisible(GYM_TRACK_TYPE track, bool visible);
void FakeAssault_SetActiveTrack(GYM_TRACK_TYPE track);
