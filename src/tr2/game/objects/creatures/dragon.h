#pragma once

#include "game/items.h"
#include "global/types.h"

void __cdecl Dragon_Collision(
    int16_t item_num, ITEM *lara_item, COLL_INFO *coll);

void __cdecl Dragon_Bones(int16_t item_num);
