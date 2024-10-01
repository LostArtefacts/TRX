#pragma once

#include <stdbool.h>
#include <stdint.h>

#define GS_DEFINE(id, value) GameString_Define(#id, value);
#define GS(id) GameString_Get(#id)
#define GS_ID(id) (#id)

typedef const char *GAME_STRING_ID;

void GameString_Define(const char *key, const char *value);
bool GameString_IsKnown(const char *key);
const char *GameString_Get(const char *key);
void GameString_Clear(void);
