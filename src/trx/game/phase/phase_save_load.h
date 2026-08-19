#pragma once

#include <trx/game/inventory_ring/enum.h>
#include <trx/game/phase/types.h>

// Reports whether the save slots for this inventory mode can be shown on
// their own, with no ring around them. Only the save and load modes qualify,
// and only while the player asks for them and the slots are usable.
bool Phase_SaveLoad_IsAvailable(INVENTORY_MODE mode);

PHASE *Phase_SaveLoad_Create(INVENTORY_MODE mode);
void Phase_SaveLoad_Destroy(PHASE *phase);
