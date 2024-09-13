#pragma once

#include "global/types.h"

// Tomb of Qualopec and Sanctuary Scion pickup.
// Triggers O_LARA_EXTRA pedestal pickup animation.

void Scion1_Setup(OBJECT_INFO *obj);
void Scion1_Collision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
