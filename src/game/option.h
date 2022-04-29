#pragma once

#include "global/types.h"

void Option_DoInventory(INVENTORY_ITEM *inv_item);

void Option_Passport(INVENTORY_ITEM *inv_item);
void Option_Compass(INVENTORY_ITEM *inv_item);
void Option_Graphics(INVENTORY_ITEM *inv_item);
void Option_Control(INVENTORY_ITEM *inv_item);
void Option_Sound(INVENTORY_ITEM *inv_item);

void Option_FlashConflicts(void);
void Option_DefaultConflict(void);

void Option_PassportInitSavegameStrings(void);
