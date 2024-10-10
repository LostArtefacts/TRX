#pragma once

#include "global/types.h"

void __cdecl Switch_Collision(
    int16_t item_num, ITEM *lara_item, COLL_INFO *coll);

void __cdecl Switch_CollisionUW(
    int16_t item_num, ITEM *lara_item, COLL_INFO *coll);

void __cdecl Switch_Control(int16_t item_num);

int32_t __cdecl Switch_Trigger(int16_t item_num, int16_t timer);
