#pragma once

#include "global/types.h"

void ThorsHammerHandle_Setup(OBJECT_INFO *obj);
void ThorsHammerHandle_Initialise(int16_t item_num);
void ThorsHammerHandle_Control(int16_t item_num);
void ThorsHammerHandle_Collision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
