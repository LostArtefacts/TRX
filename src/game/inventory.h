#pragma once

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

bool Inv_Display(const INV_MODE inv_mode);

bool Inv_AddItem(GAME_OBJECT_ID object_id);
void Inv_AddItemNTimes(GAME_OBJECT_ID object_id, int32_t qty);
void Inv_InsertItem(INVENTORY_ITEM *inv_item);
int32_t Inv_RequestItem(GAME_OBJECT_ID object_id);
void Inv_RemoveAllItems(void);
bool Inv_RemoveItem(GAME_OBJECT_ID object_id);
GAME_OBJECT_ID Inv_GetItemOption(GAME_OBJECT_ID object_id);
