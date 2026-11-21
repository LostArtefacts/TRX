#pragma once

#include <trx/game/items/types.h>
#include <trx/game/objects/general/pickup.h>

// Whether the TR4 torch lying in the world is burning.
bool BurningTorch_IsLit(const ITEM *item);
void BurningTorch_SetLit(ITEM *item, bool is_lit);

// Makes the torch fall and slide, as it does once Lara drops or throws it.
// Until then, the torch stays where the level places it, such as inside a wall
// holder, as in the original game.
void BurningTorch_Drop(ITEM *item);

PICKUP_MODE BurningTorch_GetPickupMode(const ITEM *item);
