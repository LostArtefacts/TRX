#pragma once

#include "global/types.h"

void TRex_Setup(OBJECT_INFO *obj);
void TRex_Control(int16_t item_num);
void TRex_Collision(int64_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
void TRex_LaraDeath(ITEM_INFO *item);
