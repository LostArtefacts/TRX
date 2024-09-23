#pragma once

// Main non-public control routines.

#include "global/types.h"

void Lara_HandleAboveWater(ITEM *item, COLL_INFO *coll);
void Lara_HandleUnderwater(ITEM *item, COLL_INFO *coll);
void Lara_HandleSurface(ITEM *item, COLL_INFO *coll);
