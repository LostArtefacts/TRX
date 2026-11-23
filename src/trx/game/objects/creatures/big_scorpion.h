#pragma once

#include <trx/game/items/types.h>

// Whether the big scorpion plays the animation where it kills its enemy.
bool BigScorpion_IsKilling(const ITEM *item);

// Ends the wrestle of the big scorpion with a soldier. After this, the death
// of the big scorpion does not remove the soldier.
void BigScorpion_EndWrestle(ITEM *item);
