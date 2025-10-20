#pragma once

#include "./types.h"
#include "actions/ids.h"

void Item_ActionRegister(
    ITEM_TRX_ACTION action, void (*action_func)(ITEM *item));
void Item_ActionRun(ITEM_TRX_ACTION action_id, ITEM *item);
void Item_ActionRunDirect(ITEM_ACTION action_id, ITEM *item);
void Item_ActionRunActive(void);

#define REGISTER_ITEM_ACTION(action, action_func)                              \
    __attribute__((constructor)) static void M_RegisterActionHandler##action(  \
        void)                                                                  \
    {                                                                          \
        Item_ActionRegister(action, action_func);                              \
    }
