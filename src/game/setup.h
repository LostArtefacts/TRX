#ifndef TOMB1MAIN_GAME_SETUP_H
#define TOMB1MAIN_GAME_SETUP_H

// clang-format off
#define InitialiseLevel         ((int32_t       __cdecl(*)(int32_t level_number))0x004362A0)
// clang-format on

void __cdecl BaddyObjects();
void __cdecl TrapObjects();
void __cdecl ObjectObjects();
void __cdecl InitialiseObjects();

void Tomb1MInjectGameSetup();

#endif
