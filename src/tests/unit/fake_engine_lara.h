#pragma once

#include <trx/game/lara/types.h>

#include <stdint.h>

// What the surface asked the engine to do, recorded rather than performed.
typedef struct {
    int32_t set_equipment;
    int32_t clear_equipment;
    int32_t last_mesh;
    int32_t last_extra_mesh;
} FAKE_LARA_CALLS;

extern FAKE_LARA_CALLS g_FakeLaraCalls;

void FakeLara_Reset(void);
