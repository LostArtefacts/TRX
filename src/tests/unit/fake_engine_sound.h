#pragma once

#include <stdint.h>

// The only sample the fake level has. Anything else is unavailable, which is
// what a level missing a sample looks like.
#define FAKE_SAMPLE 99
#define FAKE_MISSING_SAMPLE 1234
#define FAKE_SAMPLE_VOLUME 100
#define FAKE_SAMPLE_RANGE 8
#define FAKE_SAMPLE_RANDOMNESS 4
#define FAKE_SAMPLE_PITCH 2

// The fake's active-sound slots.
#define FAKE_SOUND_SLOT_COUNT 4

// What the surface asked the engine to do, recorded rather than performed.
typedef struct {
    int32_t play_count;
    int32_t last_sample;
    bool had_pos;
    int32_t last_x;
    int32_t last_y;
    int32_t last_z;

    int32_t stop_count;
    int32_t last_stopped_sample;
    int32_t stop_all_count;

    int32_t slot_stop_count;
    int32_t slot_stop_slot;
    int32_t slot_pause_count;
    int32_t slot_pause_slot;
    int32_t slot_unpause_count;
    int32_t slot_unpause_slot;
} FAKE_SOUND_CALLS;

extern FAKE_SOUND_CALLS g_FakeSoundCalls;

void FakeSound_Reset(void);

// Marks an active-sound slot as playing a sample.
void FakeSound_SetStream(int32_t slot, int32_t sample_id);
