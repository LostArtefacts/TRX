#ifndef T1M_GAME_LOT_H
#define T1M_GAME_LOT_H

#include <stdint.h>

// clang-format off
#define InitialiseLOT           ((void          (*)())0x0042A780)
#define InitialiseSlot          ((void          (*)(int16_t item_num, int32_t slot))0x0042A570)
// clang-format on

void InitialiseLOTArray();
void DisableBaddieAI(int16_t item_num);
int32_t EnableBaddieAI(int16_t item_num, int32_t always);

void T1MInjectGameLOT();

#endif
