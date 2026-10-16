#pragma once

#include <trx/game/objects/ids.h>

// What the game opens its menu for, which decides what the menu offers and
// what choosing an entry does.
typedef enum {
    INV_GAME_MODE,
    INV_TITLE_MODE,
    INV_KEYS_MODE,
    INV_SAVE_MODE,
    INV_LOAD_MODE,
    INV_DEATH_MODE,
    INV_SAVE_CRYSTAL_MODE,
} INVENTORY_MODE;

// Asks the next menu to open on an entry, such as the crowbar a door asks for.
void UI_RequestItem(OBJECT_ID object_id);

// Returns the entry asked for and forgets it, or NO_OBJECT where none was.
OBJECT_ID UI_TakeRequestedItem(void);
