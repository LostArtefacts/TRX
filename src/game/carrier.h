#pragma once

#include <stdint.h>

void Carrier_InitialiseLevel(int32_t level_num);
int32_t Carrier_GetItemCount(int16_t item_num);
void Carrier_TestItemDrops(int16_t item_num);
void Carrier_TestLegacyDrops(int16_t item_num);
