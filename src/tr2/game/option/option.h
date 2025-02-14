#pragma once

#include <libtrx/game/inventory_ring/types.h>

void Option_Control(INVENTORY_ITEM *item, bool is_busy);
void Option_Draw(INVENTORY_ITEM *item);
void Option_Shutdown(INVENTORY_ITEM *item);

void Option_Passport_Control(INVENTORY_ITEM *item, bool is_busy);
void Option_Passport_Draw(INVENTORY_ITEM *item);
void Option_Passport_Shutdown(void);

void Option_Detail_Control(INVENTORY_ITEM *item, bool is_busy);
void Option_Detail_Draw(INVENTORY_ITEM *item);
void Option_Detail_Shutdown(void);

void Option_Sound_Control(INVENTORY_ITEM *item, bool is_busy);
void Option_Sound_Draw(INVENTORY_ITEM *item);
void Option_Sound_Shutdown(void);

void Option_Controls_FlashConflicts(void);
void Option_Controls_DefaultConflict(void);
void Option_Controls_Control(INVENTORY_ITEM *item, bool is_busy);
void Option_Controls_Draw(INVENTORY_ITEM *item);
void Option_Controls_Shutdown(void);
void Option_Controls_ShowControls(void);
void Option_Controls_UpdateText(void);

void Option_Compass_Control(INVENTORY_ITEM *item, bool is_busy);
void Option_Compass_Draw(INVENTORY_ITEM *item);
void Option_Compass_Shutdown(void);
