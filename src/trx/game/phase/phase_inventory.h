#pragma once

#include <trx/game/phase/types.h>
#include <trx/game/ui/menu.h>

PHASE *Phase_Inventory_Create(INVENTORY_MODE mode);
void Phase_Inventory_Destroy(PHASE *phase);

// Returns whether the menu showed, which it does unless it had nothing to
// show, as for the keys of a lock when Lara carries none.
bool Phase_Inventory_IsShown(const PHASE *phase);
