#pragma once

#include <trx/game/input.h>
#include <trx/game/inventory_ring/types.h>

void Option_Controls_Control(INVENTORY_ITEM *inv_item, bool is_busy);
void Option_Controls_Draw(INVENTORY_ITEM *inv_item);
void Option_Controls_Close(void);
void Option_Controls_Shutdown(void);

// Refresh the backend picker so its visible rows match the current value of
// g_Config.input.enable_touch_controls. Safe to call at any time; does
// nothing if the controls UI hasn't been initialized yet.
void Option_Controls_RefreshBackendPicker(void);
