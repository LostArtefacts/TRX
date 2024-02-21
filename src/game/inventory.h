#pragma once

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

bool Inv_Display(const INV_MODE inv_mode);

bool Inv_AddItem(int32_t item_num);
void Inv_AddItemNTimes(int32_t item_num, int32_t qty);
void Inv_InsertItem(INVENTORY_ITEM *inv_item);
int32_t Inv_RequestItem(int item_num);
void Inv_RemoveAllItems(void);
bool Inv_RemoveItem(int32_t item_num);
int32_t Inv_GetItemOption(int32_t item_num);
