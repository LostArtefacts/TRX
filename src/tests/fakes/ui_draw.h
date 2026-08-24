#pragma once

// Records scheduled UI draw calls as text.

#include <stdint.h>

// Returns the number of recorded draw calls.
int32_t FakeUIDraw_GetCount(void);

// Returns one recorded draw call, or nullptr when index is out of range.
const char *FakeUIDraw_GetLine(int32_t index);

// Returns all recorded draw calls, one per line. Caller frees.
char *FakeUIDraw_Describe(void);

// Clears the recorded draw calls.
void FakeUIDraw_Forget(void);
