#pragma once

#include <libtrx/game/inventory_ring/types.h>

void Option_Compass_Control(INVENTORY_ITEM *inv_item, bool is_busy);
void Option_Compass_Draw(void);
void Option_Compass_Shutdown(void);

void Option_Compass_UpdateNeedle(const INVENTORY_ITEM *inv_item);
int16_t Option_Compass_GetNeedleAngle(void);
