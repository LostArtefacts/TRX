#pragma once

#include <trx/game/inventory_ring/types.h>

void Option_Combine_Control(INVENTORY_ITEM *inv_item, bool is_busy);
void Option_Combine_Draw(void);
void Option_Combine_Close(void);

// Return the object chosen for combination, or NO_OBJECT if no object was
// chosen. Clear the choice after the call.
OBJECT_ID Option_Combine_TakeChoice(void);
