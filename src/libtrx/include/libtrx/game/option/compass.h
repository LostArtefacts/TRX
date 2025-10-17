#pragma once

#include "../inventory_ring/types.h"

extern void Option_Compass_Control(INVENTORY_ITEM *inv_item, bool is_busy);
extern void Option_Compass_Draw(void);
extern void Option_Compass_Close(void);

void Option_Compass_UpdateNeedle(const INVENTORY_ITEM *inv_item);
int16_t Option_Compass_GetNeedleAngle(void);
