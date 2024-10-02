#pragma once

#include "global/types.h"

void TRex_Setup(OBJECT *obj);
void TRex_Control(int16_t item_num);
void TRex_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
void TRex_LaraDeath(ITEM *item);
