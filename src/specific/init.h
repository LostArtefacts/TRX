#ifndef TOMB1MAIN_SPECIFIC_INIT_H
#define TOMB1MAIN_SPECIFIC_INIT_H

// clang-format off
#define game_malloc             ((void          __cdecl*(*)(uint32_t length, int type))0x0041E2F0)
// clang-format on

void __cdecl init_game_malloc();
void __cdecl game_free(int free_size);

void Tomb1MInjectSpecificInit();

#endif
