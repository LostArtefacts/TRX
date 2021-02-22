#ifndef T1M_SPECIFIC_INIT_H
#define T1M_SPECIFIC_INIT_H

// clang-format off
#define game_malloc             ((void          __cdecl*(*)(uint32_t length, int type))0x0041E2F0)
// clang-format on

void __cdecl S_InitialiseSystem();
void __cdecl init_game_malloc();
void __cdecl game_free(int free_size);
void __cdecl CalculateWibbleTable();
void __cdecl S_SeedRandom();

void T1MInjectSpecificInit();

#endif
