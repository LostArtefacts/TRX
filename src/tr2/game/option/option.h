#pragma once

#include <libtrx/game/inventory_ring/types.h>

void Option_Control(INVENTORY_ITEM *inv_item, bool is_busy);
void Option_Draw(INVENTORY_ITEM *inv_item);
void Option_Close(INVENTORY_ITEM *inv_item);

void Option_Passport_Control(INVENTORY_ITEM *inv_item, bool is_busy);
void Option_Passport_Draw(INVENTORY_ITEM *inv_item);
void Option_Passport_Close(void);

void Option_Controls_FlashConflicts(void);
void Option_Controls_DefaultConflict(void);
void Option_Controls_Control(INVENTORY_ITEM *inv_item, bool is_busy);
void Option_Controls_Draw(INVENTORY_ITEM *inv_item);
void Option_Controls_Close(void);
void Option_Controls_ShowControls(void);
void Option_Controls_UpdateText(void);

void Option_Compass_Control(INVENTORY_ITEM *inv_item, bool is_busy);
void Option_Compass_Draw(void);
void Option_Compass_Close(void);
