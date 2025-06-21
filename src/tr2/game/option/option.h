#pragma once

#include <libtrx/game/option/common.h>

void Option_Passport_Control(INVENTORY_ITEM *inv_item, bool is_busy);
void Option_Passport_Draw(INVENTORY_ITEM *inv_item);
void Option_Passport_Close(void);

void Option_Compass_Control(INVENTORY_ITEM *inv_item, bool is_busy);
void Option_Compass_Draw(void);
void Option_Compass_Close(void);
