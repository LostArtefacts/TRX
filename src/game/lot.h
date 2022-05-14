#pragma once

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

void LOT_InitialiseArray(void);
void LOT_DisableBaddieAI(int16_t item_num);
bool LOT_EnableBaddieAI(int16_t item_num, int32_t always);
void InitialiseSlot(int16_t item_num, int32_t slot);
void CreateZone(ITEM_INFO *item);
int32_t InitialiseLOT(LOT_INFO *LOT);
void ClearLOT(LOT_INFO *LOT);
