#ifndef T1M_GAME_LOT_H
#define T1M_GAME_LOT_H

#include "game/types.h"
#include <stdint.h>

// clang-format off
#define InitialiseLOT           ((void          (*)())0x0042A780)
// clang-format on

void InitialiseLOTArray();
void DisableBaddieAI(int16_t item_num);
int32_t EnableBaddieAI(int16_t item_num, int32_t always);
void InitialiseSlot(int16_t item_num, int32_t slot);
void CreateZone(ITEM_INFO* item);
void ClearLOT(LOT_INFO* LOT);

void T1MInjectGameLOT();

#endif
