#pragma once

#include <trx/game/lara/types.h>

#include <lualib.h>
#include <stdint.h>

// What the surface asked the engine to do, recorded rather than performed.
typedef struct {
    int32_t set_equipment;
    int32_t clear_equipment;
    int32_t last_mesh;
    int32_t last_extra_mesh;
    int32_t cure_poison;
    int32_t extinguish;
    int32_t catch_fire;
    int32_t dry;
    int32_t teleport;
    int32_t last_teleport_room;
} FAKE_LARA_CALLS;

extern FAKE_LARA_CALLS g_FakeLaraCalls;

void FakeLara_Reset(void);

// Adds what Lara was asked to do to the table on top of the stack.
void FakeLara_PushCalls(lua_State *L);
