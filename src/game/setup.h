#ifndef T1M_GAME_SETUP_H
#define T1M_GAME_SETUP_H

#include <stdint.h>

// clang-format off
#define InitialiseLevel         ((int32_t      (*)(int32_t level_number))0x004362A0)
// clang-format on

void BaddyObjects();
void TrapObjects();
void ObjectObjects();
void InitialiseObjects();

void T1MInjectGameSetup();

#endif
