#ifndef TR1MAIN_GAME_SETUP_H
#define TR1MAIN_GAME_SETUP_H

// clang-format off
#define BaddyObjects            ((void          __cdecl(*)())0x004363E0)
#define TrapObjects             ((void          __cdecl(*)())0x00437010)
#define ObjectObjects           ((void          __cdecl(*)())0x00437370)
// clang-format on

void __cdecl InitialiseObjects();

void TR1MInjectSetup();

#endif
