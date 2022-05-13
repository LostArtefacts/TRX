#pragma once

#include "global/types.h"

#include <stdint.h>

void Door_Setup(OBJECT_INFO *obj);
void Door_Initialise(int16_t item_num);
void Door_Control(int16_t item_num);
void Door_Collision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
void Door_OpenNearest(ITEM_INFO *lara_item);
