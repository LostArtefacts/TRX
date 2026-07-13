#pragma once

#include <stdint.h>

#define FAKE_CAMERA_ROOM 2
#define FAKE_CAMERA_TARGET_ROOM 3

// What the surface asked the engine to do, recorded rather than performed.
typedef struct {
    int32_t reset;
    int32_t shake;
} FAKE_CAMERA_CALLS;

extern FAKE_CAMERA_CALLS g_FakeCameraCalls;

void FakeCamera_Reset(void);

// Puts the camera in a room that does not exist, which is how the engine says
// "nowhere".
void FakeCamera_SetNoRoom(void);
