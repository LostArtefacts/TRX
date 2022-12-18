#pragma once

#include "global/types.h"

#include <stdbool.h>

bool Option_ControlIsLocked(void);
CONTROL_MODE Option_Control(INVENTORY_ITEM *inv_item, CONTROL_MODE mode);
