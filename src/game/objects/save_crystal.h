#pragma once

#include "global/types.h"

#include <stdint.h>

void SaveCrystal_Setup(OBJECT_INFO *obj);
void SaveCrystal_Initialise(int16_t item_num);
void SaveCrystal_Control(int16_t item_num);
void SaveCrystal_Collision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
