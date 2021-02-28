#ifndef T1M_GAME_SETUP_H
#define T1M_GAME_SETUP_H

#include <stdint.h>

// clang-format off
#define InitialiseLevelFlags    ((void          (*)())0x004363C0)
// clang-format on

int32_t InitialiseLevel(int32_t level_num);
void InitialiseGameFlags();
void BaddyObjects();
void TrapObjects();
void ObjectObjects();
void InitialiseObjects();

void T1MInjectGameSetup();

#endif
