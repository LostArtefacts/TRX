#pragma once

#include <trx/game/inventory_ring/types.h>

void Option_SaveCrystal_Control(INVENTORY_ITEM *inv_item, bool is_busy);
void Option_SaveCrystal_Draw(void);
void Option_SaveCrystal_Close(void);

// Saves the game in the slot the player chose and spends a crystal on it.
void Option_SaveCrystal_CommitSave(void);
