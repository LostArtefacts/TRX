#pragma once

#include <trx/game/items/types.h>

typedef struct {
    bool drawn;
    bool shadow_drawn;
} OUTPUT_ITEM_BIND;

void Output_Bind_ResetItems(void);
OUTPUT_ITEM_BIND *Output_Bind_GetItem(const ITEM *item);
