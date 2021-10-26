#ifndef T1M_SPECIFIC_INIT_H
#define T1M_SPECIFIC_INIT_H

#include "global/types.h"

#include <stdint.h>

void DB_Log(const char *fmt, ...);

void S_InitialiseSystem();
void S_ExitSystem(const char *message);

void CalculateWibbleTable();
void S_SeedRandom();

void init_game_malloc();
void *game_malloc(int32_t alloc_size, GAMEALLOC_BUFFER buf_index);
void game_free(int32_t free_size, int32_t type);

void T1MInjectSpecificInit();

#endif
