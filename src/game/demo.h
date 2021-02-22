#ifndef TOMB1MAIN_GAME_DEMO_H
#define TOMB1MAIN_GAME_DEMO_H

#include <stdint.h>

// clang-format off
#define GetDemoInput            ((void          __cdecl(*)())0x00415D70)
#define LoadLaraDemoPos         ((void          __cdecl(*)())0x00415CB0)
// clang-format on

int32_t __cdecl StartDemo();

void T1MInjectGameDemo();

#endif
