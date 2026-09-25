#pragma once

#include <trx/game/inventory_ring/types.h>

// Whether the game offers a save once Lara is free to move. A level sets it
// as it starts when the save crystal mode asks for a save on entry.
extern bool g_SaveCrystal_AskForSave;

void Option_SaveCrystal_Control(INVENTORY_ITEM *inv_item, bool is_busy);
void Option_SaveCrystal_Draw(void);
void Option_SaveCrystal_Close(void);

// Saves the game in the slot the player chose and spends a crystal on it.
void Option_SaveCrystal_CommitSave(void);
