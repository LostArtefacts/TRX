#pragma once

#include <stdint.h>

#define GS_DEFINE(id, value) GameString_Define(#id, value);
#define GS(id) GameString_Get(#id)
#define GS_ID(id) (#id)

typedef const char *GAME_STRING_ID;

void GameString_Define(const char *key, const char *value);
bool GameString_IsKnown(const char *key);
const char *GameString_Get(const char *key);
void GameString_Clear(void);
void GameString_Shutdown(void);

// Like GameString_Get(), but returns a stable slot pointer that always
// reflects the current value for this key (updates automatically on reload).
const char *const *GameString_GetPtr(const char *key);

// Macro to get the slot pointer for a compile-time string ID.
#define GS_PTR(id) GameString_GetPtr(#id)
