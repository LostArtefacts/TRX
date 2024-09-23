#pragma once

#include "global/types.h"

void SaveCrystal_Setup(OBJECT *obj);
void SaveCrystal_Initialise(int16_t item_num);
void SaveCrystal_Control(int16_t item_num);
void SaveCrystal_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
