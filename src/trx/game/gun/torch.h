#pragma once

#include <trx/game/collision/types.h>
#include <trx/game/items/types.h>

// Puts the TR4 torch in Lara's left hand, burning or not, as she picks one up.
void Gun_Torch_Take(bool is_lit);

// Lets Lara light her torch from a fire, or light the fire from her torch,
// as she stands still before it with the action key held.
void Gun_Torch_FireCollision(
    int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
