#ifndef T1M_GAME_DEMO_H
#define T1M_GAME_DEMO_H

#include <stdint.h>

// clang-format off
#define GetDemoInput            ((void         (*)())0x00415D70)
#define LoadLaraDemoPos         ((void         (*)())0x00415CB0)
// clang-format on

int32_t StartDemo();

void T1MInjectGameDemo();

#endif
