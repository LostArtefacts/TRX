#pragma once

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
