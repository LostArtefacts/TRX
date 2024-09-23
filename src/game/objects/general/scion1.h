#pragma once

#include "global/types.h"

// Tomb of Qualopec and Sanctuary Scion pickup.
// Triggers O_LARA_EXTRA pedestal pickup animation.

void Scion1_Setup(OBJECT *obj);
void Scion1_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
