#ifndef T1M_SPECIFIC_INIT_H
#define T1M_SPECIFIC_INIT_H

#include "global/types.h"

#include <stdarg.h>
#include <stdint.h>

void S_InitialiseSystem();
void S_ExitSystem(const char *message);
void S_ExitSystemFmt(const char *fmt, ...);

void CalculateWibbleTable();
void S_SeedRandom();

void init_game_malloc();
void *game_malloc(int32_t alloc_size, GAMEALLOC_BUFFER buf_index);
void game_free(int32_t free_size, int32_t type);
void game_malloc_shutdown();

#endif
