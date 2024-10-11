#pragma once

#include "global/types.h"

void __cdecl Keyhole_Collision(
    int16_t item_num, ITEM *lara_item, COLL_INFO *coll);

int32_t __cdecl Keyhole_Trigger(int16_t item_num);
