#pragma once

#include <trx/game/catalog/manager.h>

#include <stdint.h>

// Identify an item action by its number in a game's files.
typedef int32_t ITEM_ACTION_SLOT;

enum {
    ITEM_ACTION_INVALID = -1,
};

// Identify the same item action across all four games.
typedef CATALOG_ID ITEM_ACTION_ID;

enum {
#define X_CATALOG_ID(enum_value) enum_value,
#include <trx/game/catalog/item_actions.def>
#undef X_CATALOG_ID
};

ITEM_ACTION_SLOT ItemAction_IDToSlot(ITEM_ACTION_ID id);
ITEM_ACTION_ID ItemAction_SlotToID(ITEM_ACTION_SLOT slot);
