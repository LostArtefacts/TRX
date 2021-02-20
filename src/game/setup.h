#ifndef TOMB1MAIN_GAME_SETUP_H
#define TOMB1MAIN_GAME_SETUP_H

// clang-format off
#define BaddyObjects            ((void          __cdecl(*)())0x004363E0)
#define TrapObjects             ((void          __cdecl(*)())0x00437010)
#define ObjectObjects           ((void          __cdecl(*)())0x00437370)
#define InitialiseLevel         ((int32_t       __cdecl(*)(int32_t level_number))0x004362A0)
// clang-format on

void __cdecl InitialiseObjects();

void Tomb1MInjectGameSetup();

#endif
