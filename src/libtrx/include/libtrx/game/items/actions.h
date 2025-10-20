#pragma once

#include "./types.h"
#include "actions/ids.h"

extern void Item_ActionRun(ITEM_TRX_ACTION action_id, ITEM *item);
void Item_ActionRunDirect(ITEM_ACTION action_id, ITEM *item);
void Item_ActionRunActive(void);
