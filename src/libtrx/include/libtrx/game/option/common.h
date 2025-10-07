#pragma once

#include "../inventory_ring/types.h"

extern void Option_Control(INVENTORY_ITEM *inv_item, bool is_busy);
extern void Option_Draw(INVENTORY_ITEM *inv_item);
extern void Option_Close(const INVENTORY_ITEM *inv_item);

// Reset internal positioning of option UIs.
void Option_Reset(void);

// Free up resources associated with option UIs.
void Option_Shutdown(void);
