#ifndef T1M_GAME_LOT_H
#define T1M_GAME_LOT_H

#include <stdint.h>

// clang-format off
#define InitialiseLOT           ((void          (*)())0x0042A780)
#define EnableBaddieAI          ((int32_t       (*)(int16_t item_num, int32_t))0x0042A3A0)
// clang-format on

void InitialiseLOTArray();
void DisableBaddieAI(int16_t item_num);

void T1MInjectGameLOT();

#endif
