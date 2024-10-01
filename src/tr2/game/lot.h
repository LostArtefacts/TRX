#pragma once

#include "global/types.h"

void __cdecl LOT_InitialiseArray(void);
void __cdecl LOT_DisableBaddieAI(int16_t item_num);
bool __cdecl LOT_EnableBaddieAI(int16_t item_num, bool always);
void __cdecl LOT_InitialiseSlot(int16_t item_num, int32_t slot);
void __cdecl LOT_CreateZone(ITEM *item);
void __cdecl LOT_ClearLOT(LOT_INFO *LOT);
