#pragma once

#include "global/types.h"

void __cdecl Skidoo_Initialise(int16_t item_num);
int32_t __cdecl Skidoo_CheckGetOn(int16_t item_num, COLL_INFO *coll);
void __cdecl Skidoo_Collision(
    int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
