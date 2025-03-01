#pragma once

#include "inventory_ring/types.h"
#include "lara/types.h"
#include "objects/ids.h"

#include <stdint.h>

bool Inv_AddItemNTimes(GAME_OBJECT_ID obj_id, int32_t qty);
void Inv_AddAmmo(AMMO_INFO *weapon_ammo, int32_t qty);
GAME_OBJECT_ID Inv_GetItemOption(GAME_OBJECT_ID obj_id);
void Inv_InsertItem(INVENTORY_ITEM *inv_item);
bool Inv_RemoveItem(GAME_OBJECT_ID obj_id);
int32_t Inv_RequestItem(GAME_OBJECT_ID obj_id);
void Inv_ClearSelection(void);
void Inv_RemoveAllItems(void);

extern bool Inv_AddItem(GAME_OBJECT_ID obj_id);
