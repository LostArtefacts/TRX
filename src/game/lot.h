#pragma once

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

void LOT_InitialiseArray(void);
void LOT_DisableBaddieAI(int16_t item_num);
bool LOT_EnableBaddieAI(int16_t item_num, int32_t always);
void LOT_InitialiseSlot(int16_t item_num, int32_t slot);
void LOT_CreateZone(ITEM_INFO *item);
void LOT_InitialiseLOT(LOT_INFO *LOT);
void ClearLOT(LOT_INFO *LOT);
