#pragma once

#include <stdint.h>

#define FAKE_ROOM_COUNT 4

// The rooms of the next level replace the ones a handle was taken from.
void FakeRooms_LoadNextLevel(void);
