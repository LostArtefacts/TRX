#pragma once

#define GS(id) GameString_Get(GS_##id)

#undef GS_DEFINE
#define GS_DEFINE(id, str) GS_##id,
typedef enum GAME_STRING_ID {
    GS_INVALID = -1,
#include "game/game_string.def"
    GS_NUMBER_OF,
} GAME_STRING_ID;

void GameString_Set(GAME_STRING_ID id, const char *value);
const char *GameString_Get(GAME_STRING_ID id);
GAME_STRING_ID GameString_IDFromEnum(const char *str);
