#pragma once

#include "global/types.h"

void __cdecl Door_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
void __cdecl Object_Collision_Trap(
    int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
