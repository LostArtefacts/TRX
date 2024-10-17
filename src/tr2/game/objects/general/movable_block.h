#pragma once

#include "global/types.h"

int32_t __cdecl MovableBlock_TestDestination(
    const ITEM *item, int32_t block_height);

void __cdecl MovableBlock_Initialise(int16_t item_num);

void __cdecl MovableBlock_Control(int16_t item_num);

void __cdecl MovableBlock_Collision(
    int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
