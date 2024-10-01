#pragma once

#include "global/types.h"

// Atlantis Scion - triggers O_LARA_EXTRA reach anim.

void Scion4_Setup(OBJECT *obj);
void Scion4_Control(int16_t item_num);
void Scion4_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
