#pragma once

#include <stdbool.h>
#include <stdint.h>

#define FAKE_ITEM_POOL 8 // small, so pool exhaustion is reachable
#define FAKE_OBJ_WOLF 1 // intelligent, has animations
#define FAKE_OBJ_VASE 2 // inert scenery
#define FAKE_OBJ_UNLOADED 3 // declared but not loaded

// What the surface asked the engine to do, recorded rather than performed.
typedef struct {
    int32_t swap_mesh;
    int32_t creature_die;
    bool creature_die_explode;
    int32_t enable_baddie_ai;
    int32_t kill;
} FAKE_ITEM_CALLS;

extern FAKE_ITEM_CALLS g_FakeItemCalls;

void FakeItems_Reset(void);
