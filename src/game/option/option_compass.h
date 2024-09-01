#pragma once

#include "global/types.h"

void Option_Compass(INVENTORY_ITEM *inv_item);
void Option_Compass_Shutdown(void);

void Option_Compass_UpdateNeedle(const INVENTORY_ITEM *inv_item);
int16_t Option_Compass_GetNeedleAngle(void);
