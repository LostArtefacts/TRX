#pragma once

#include <trx/game/gym.h>

#include <stdint.h>

// Outside a gym level the timers do not exist, and every verb raises.
void FakeAssault_SetInGym(bool in_gym);
void FakeAssault_SetHasStats(GYM_TRACK_TYPE track, bool has_stats);
void FakeAssault_SetRunning(GYM_TRACK_TYPE track, bool running);
void FakeAssault_SetVisible(GYM_TRACK_TYPE track, bool visible);
void FakeAssault_SetActiveTrack(GYM_TRACK_TYPE track);
