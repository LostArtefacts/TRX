#pragma once

#include <stdint.h>

#define FAKE_ROOM_COUNT 4

// What the surface asked the engine to do, recorded rather than performed.
typedef struct {
    int32_t flip_map;
    int32_t flip_effect;
    int32_t flip_timer;
    bool fix_tilts;
} FAKE_ROOM_CALLS;

extern FAKE_ROOM_CALLS g_FakeRoomCalls;

void FakeRooms_Reset(void);

// The rooms of the next level replace the ones a handle was taken from.
void FakeRooms_LoadNextLevel(void);
