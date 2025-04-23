#pragma once

#include "../creature/types.h"

extern bool LOT_EnableBaddieAI(int16_t item_num, bool always);
extern void LOT_DisableBaddieAI(int16_t item_num);
extern CREATURE *LOT_GetBaddieSlot(int32_t i);
extern void LOT_ClearLOT(LOT_INFO *LOT);
