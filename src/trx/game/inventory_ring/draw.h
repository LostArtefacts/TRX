#pragma once

#include <trx/game/inventory_ring/types.h>

void InvRing_Draw(INV_RING *ring);

// Draws the items around a ring and nothing else, leaving the view as it was.
// A ring drawn over another one is drawn with this.
void InvRing_DrawItems(INV_RING *ring);

INVENTORY_ITEM *InvRing_GetInvItem(OBJECT_ID obj_id);
