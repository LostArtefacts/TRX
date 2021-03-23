#ifndef T1M_SPECIFIC_INIT_H
#define T1M_SPECIFIC_INIT_H

#include <stdint.h>

// clang-format off
#define game_malloc             ((void*         (*)(uint32_t length, int32_t type))0x0041E2F0)
// clang-format on

void DB_Log(const char *fmt, ...);
void S_InitialiseSystem();
void init_game_malloc();
void game_free(int32_t free_size, int32_t type);
void CalculateWibbleTable();
void S_SeedRandom();

void T1MInjectSpecificInit();

#endif
