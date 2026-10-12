#pragma once

// Records scheduled UI draw calls as text.

#include <trx/game/ui/common.h>

#include <stdint.h>

// Sets how far a drawn frame sits between two ticks, which is what a model
// kept in a slot is blended by. Zero is the tick it starts from.
void FakeUIDraw_SetInterpolationRate(double rate);

// Records one line, for a leaf a test draws itself.
void FakeUIDraw_Record(const char *line);

// Returns the number of recorded draw calls.
int32_t FakeUIDraw_GetCount(void);

// Returns one recorded draw call, or nullptr when index is out of range.
const char *FakeUIDraw_GetLine(int32_t index);
// Returns the layer a recorded draw call was scheduled in.
UI_LAYER FakeUIDraw_GetLayer(int32_t index);
// Returns the name of that layer, as tests spell it.
const char *FakeUIDraw_GetLayerName(int32_t index);

// Returns all recorded draw calls, one per line. Caller frees.
char *FakeUIDraw_Describe(void);

// Clears the recorded draw calls.
void FakeUIDraw_Forget(void);
