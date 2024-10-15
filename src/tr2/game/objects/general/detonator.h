#pragma once

#include "global/types.h"

void __cdecl Detonator_Control(int16_t item_num);

void __cdecl Detonator_Collision(
    int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
