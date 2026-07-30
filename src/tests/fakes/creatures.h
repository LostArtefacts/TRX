#pragma once

#include <stdint.h>

// What the surface asked the engine to do, recorded rather than performed.
typedef struct {
    int32_t add_ally;
    int32_t add_ally_target;
    int32_t last_ally_object_id;
    int32_t last_ally_target_object_id;
} FAKE_CREATURE_CALLS;

extern FAKE_CREATURE_CALLS g_FakeCreatureCalls;

void FakeCreatures_Reset(void);
