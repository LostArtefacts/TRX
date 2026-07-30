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

// Marks an active-sound slot as playing a sample.
void FakeSound_SetStream(int32_t slot, int32_t sample_id);
