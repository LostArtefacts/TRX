#ifndef T1M_GAME_GAMEFLOW_H
#define T1M_GAME_GAMEFLOW_H

#include "game/types.h"

#include <stdint.h>

// T1M: gameflow implementation.

GAMEFLOW_OPTION
GF_InterpretSequence(int32_t level_num, GAMEFLOW_LEVEL_TYPE level_type);
int8_t GF_LoadScriptFile(const char *file_name);

#endif
