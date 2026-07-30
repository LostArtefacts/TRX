#pragma once

#include <stdint.h>

#define FAKE_CAMERA_ROOM 2
#define FAKE_CAMERA_TARGET_ROOM 3

// Puts the camera in a room that does not exist, which is how the engine says
// "nowhere".
void FakeCamera_SetNoRoom(void);

// Whether a flyby sequence is playing.
void FakeCamera_SetFlybyActive(bool active);
