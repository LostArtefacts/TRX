#pragma once

#include "global/types.h"

void __cdecl Inv_InsertItem(INVENTORY_ITEM *inv_item);
int32_t __cdecl Inv_AddItem(GAME_OBJECT_ID object_id);
void Inv_AddItemNTimes(GAME_OBJECT_ID object_id, int32_t qty);
int32_t __cdecl Inv_RequestItem(GAME_OBJECT_ID object_id);
void __cdecl Inv_RemoveAllItems(void);
int32_t __cdecl Inv_RemoveItem(GAME_OBJECT_ID object_id);
