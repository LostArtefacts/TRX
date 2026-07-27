#pragma once

#include <stdint.h>

#define FAKE_ITEM_POOL 8 // small, so pool exhaustion is reachable
#define FAKE_OBJ_WOLF 1 // intelligent, has animations
#define FAKE_OBJ_VASE 2 // inert scenery, and a pickup
#define FAKE_OBJ_UNLOADED 3 // declared but not loaded
#define FAKE_OBJ_KEY 4 // a second pickup, so a group name matches more than one

// What the surface asked the engine to do, recorded rather than performed.
typedef struct {
    int32_t swap_mesh;
    int32_t creature_die;
    bool creature_die_explode;
    int32_t shatter;
    int16_t shatter_damage;
    int32_t enable_baddie_ai;
    bool enable_baddie_ai_forced;
    int32_t disable_baddie_ai;
    int32_t destroy;
    int32_t take_damage;
    int16_t take_damage_amount;
} FAKE_ITEM_CALLS;

extern FAKE_ITEM_CALLS g_FakeItemCalls;

void FakeItems_Reset(void);
