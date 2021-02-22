#ifndef T1M_SPECIFIC_INIT_H
#define T1M_SPECIFIC_INIT_H

// clang-format off
#define game_malloc             ((void         *(*)(uint32_t length, int type))0x0041E2F0)
// clang-format on

void S_InitialiseSystem();
void init_game_malloc();
void game_free(int free_size);
void CalculateWibbleTable();
void S_SeedRandom();

void T1MInjectSpecificInit();

#endif
