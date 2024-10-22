#pragma once

#include "global/types.h"

void SkidooArmed_Setup(void);

void __cdecl SkidooArmed_Push(
    const ITEM *item, ITEM *lara_item, int32_t radius);

void __cdecl SkidooArmed_Collision(
    int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
