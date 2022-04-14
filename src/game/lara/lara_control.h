#pragma once

// Main non-public control routines.

#include "global/types.h"

void Lara_HandleAboveWater(ITEM_INFO *item, COLL_INFO *coll);
void Lara_HandleUnderwater(ITEM_INFO *item, COLL_INFO *coll);
void Lara_HandleSurface(ITEM_INFO *item, COLL_INFO *coll);
void Lara_CheatGetStuff(void);
