#ifndef T1M_GAME_OPTION_H
#define T1M_GAME_OPTION_H

#include "global/types.h"

void Option_DoInventory(INVENTORY_ITEM *inv_item);

void Option_DoPassport(INVENTORY_ITEM *inv_item);
void Option_DoCompass(INVENTORY_ITEM *inv_item);
void Option_DoDetail(INVENTORY_ITEM *inv_item);
void Option_Control(INVENTORY_ITEM *inv_item);
void Option_FlashConflicts();
void Option_DefaultConflict();
void Option_DoSound(INVENTORY_ITEM *inv_item);
void Option_ControlShowControls();
void Option_ControlUpdateText();
void Option_ControlCleanup();

#endif
