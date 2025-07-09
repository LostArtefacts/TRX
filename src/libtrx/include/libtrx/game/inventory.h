#pragma once

#include "inventory_ring/types.h"
#include "lara/types.h"
#include "objects/ids.h"

#include <stdint.h>

extern INVENTORY_MODE g_Inv_Mode;

void Inv_AddGun(LARA_GUN_TYPE gun_type);
void Inv_AddAmmo(LARA_GUN_TYPE gun_type);

bool Inv_AddItemNTimes(GAME_OBJECT_ID obj_id, int32_t qty);
GAME_OBJECT_ID Inv_GetItemOption(GAME_OBJECT_ID obj_id);
void Inv_InsertItem(INVENTORY_ITEM *inv_item);
bool Inv_RemoveItem(GAME_OBJECT_ID obj_id);
int32_t Inv_RequestItem(GAME_OBJECT_ID obj_id);
void Inv_ClearSelection(void);
void Inv_RemoveAllItems(void);

extern bool Inv_AddItem(GAME_OBJECT_ID obj_id);
extern bool Inv_AddPickup(const ITEM *item);
